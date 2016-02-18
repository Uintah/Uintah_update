/*
 * The MIT License
 *
 * Copyright (c) 1997-2014 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <CCA/Components/Schedulers/ThreadFunneledScheduler.h>
#include <CCA/Components/Schedulers/OnDemandDataWarehouse.h>
#include <CCA/Components/Schedulers/TaskGraph.h>
#include <CCA/Ports/Output.h>

#include <Core/Exceptions/ProblemSetupException.h>

#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

#include <sched.h>

#define USE_PACKING

using namespace Uintah;

extern std::map<std::string, double> waittimes;
extern std::map<std::string, double> exectimes;

extern DebugStream execout;
extern DebugStream taskdbg;
extern DebugStream timeout;
extern DebugStream waitout;

//______________________________________________________________________
//
namespace {

DebugStream threaded_dbg(       "ThreadFunneled_DBG",        false);
DebugStream threaded_threaddbg( "ThreadFunneled_ThreadDBG",  false);

Lockfree::Timer s_total_exec_time {};
std::mutex      s_io_mutex;
std::mutex      s_lb_mutex;

double                 g_thread_funneled_current_wait_time = 0;

} // namespace


//______________________________________________________________________
//
namespace Uintah { namespace Impl {
namespace {

enum class ThreadState : int
{
    Inactive
  , Active
  , Exit
};

TaskRunner *           g_runners[MAX_THREADS]        = {};
volatile ThreadState   g_thread_states[MAX_THREADS]  = {};
int                    g_cpu_affinities[MAX_THREADS] = {};
int                    g_num_threads                 = 0;

thread_local int       t_tid = 0;


//______________________________________________________________________
//
void set_affinity( const int proc_unit )
{
#ifndef __APPLE__
  //disable affinity on OSX since sched_setaffinity() is not available in OSX API
  cpu_set_t mask;
  unsigned int len = sizeof(mask);
  CPU_ZERO(&mask);
  CPU_SET(proc_unit, &mask);
  sched_setaffinity(0, len, &mask);
#endif
}


//______________________________________________________________________
//
void thread_driver( const int tid )
{
  // t_tid is thread_local variable, unique to each std::thread spawned below
  t_tid = tid;

  // set each TaskWorker thread's affinity
  set_affinity( g_cpu_affinities[tid] );

  try {
    // wait until master sets function and changes states
    g_thread_states[tid] = ThreadState::Inactive;
    while (g_thread_states[tid] == ThreadState::Inactive) {
      std::this_thread::yield();
    }

    while (g_thread_states[tid] == ThreadState::Active) {

      // run the function and wait for master to reset state
      g_runners[tid]->run();

      g_thread_states[tid] = ThreadState::Inactive;
      while (g_thread_states[tid] == ThreadState::Inactive) {
        std::this_thread::yield();
      }
    }
  } catch (const std::exception & e) {
    std::cerr << "Exception thrown from worker thread: " << e.what() << std::endl;
    std::cerr.flush();
    std::abort();
  } catch (...) {
    std::cerr << "Unknown Exception thrown from worker thread" << std::endl;
    std::cerr.flush();
    std::abort();
  }
}


//______________________________________________________________________
// only called by thread 0 (master)
void thread_fence()
{
  // master thread tid is at [0]
  g_thread_states[0] = ThreadState::Inactive;

  // TaskRunner threads start at [1]
  for (int i = 1; i < g_num_threads; ++i) {
    while (g_thread_states[i] == ThreadState::Active) {
      // std::this_thread::sleep_for( ... )
      // backoff
      std::this_thread::yield();
    }
  }
  std::atomic_thread_fence(std::memory_order_seq_cst);
}


//______________________________________________________________________
// only called by thread 0 (master)
void init_threads( ThreadFunneledScheduler * sched, int num_threads )
{
  g_num_threads = num_threads;
  for (int i = 0; i < g_num_threads; ++i) {
    g_thread_states[i]  = ThreadState::Active;
    g_cpu_affinities[i] = i;
  }

  // set master thread's affinity - core 0
  set_affinity(g_cpu_affinities[0]);

  // TaskRunner threads start at [1]
  for (int i = 1; i < g_num_threads; ++i) {
    g_runners[i] = new TaskRunner(sched);
  }

  // spawn worker threads
  // TaskRunner threads start at [1]
  for (int i = 1; i < g_num_threads; ++i) {
    std::thread(thread_driver, i).detach();
  }

  thread_fence();
}

} // namespace
}} // namespace Uintah::Impl


//______________________________________________________________________
//
ThreadFunneledScheduler::~ThreadFunneledScheduler()
{
  // detailed MPI information, written to file per rank
  if (timeout.active()) {
    timingStats.close();
    if (d_myworld->myrank() == 0) {
      avgStats.close();
      maxStats.close();
    }
  }
}


//______________________________________________________________________
//
void ThreadFunneledScheduler::problemSetup(  const ProblemSpecP     & prob_spec
                                           ,       SimulationStateP & state
                                          )
{
  std::string taskQueueAlg = "MostMessages"; // default taskReadyQueueAlg

  proc0cout << "   Using \"" << taskQueueAlg << "\" task queue priority algorithm" << std::endl;

  m_num_threads = Uintah::Parallel::getNumThreads();
  if ((m_num_threads < 1) && Uintah::Parallel::usingMPI()) {
    if (d_myworld->myrank() == 0) {
      std::cerr << "Error: no thread number specified for ThreadedMPIScheduler" << std::endl;
      throw ProblemSetupException("This scheduler requires number of threads to be in the range [2, 64],\n.... please use -nthreads <num>", __FILE__, __LINE__);
      }
    }
  else if (m_num_threads > MAX_THREADS) {
    if (d_myworld->myrank() == 0) {
      std::cerr << "Error: Number of threads too large..." << std::endl;
      throw ProblemSetupException("Too many threads. Reduce MAX_THREADS and recompile.", __FILE__, __LINE__);
    }
  }

  if (d_myworld->myrank() == 0) {
    std::string plural = (m_num_threads == 1) ? " thread" : " threads";
    std::cout << "   WARNING: Component tasks must be thread safe.\n"
              << "   Using 1 thread for scheduling, and " << m_num_threads
              << plural + " for task execution." << std::endl;
  }

  // this spawns threads, sets affinity, etc
  init_threads(this, m_num_threads);

  log.problemSetup(prob_spec);
  SchedulerCommon::problemSetup(prob_spec, state);
}


//______________________________________________________________________
//
SchedulerP ThreadFunneledScheduler::createSubScheduler()
{
  ThreadFunneledScheduler* newsched = new ThreadFunneledScheduler(d_myworld, m_outPort, this);
  newsched->d_sharedState = d_sharedState;
  UintahParallelPort* lbp = getPort("load balancer");
  newsched->attachPort("load balancer", lbp);
  newsched->d_sharedState = d_sharedState;
  return newsched;
}


//______________________________________________________________________
//
void ThreadFunneledScheduler::execute(  int tgnum     /*=0*/ , int iteration /*=0*/ )
{
  // must be single threaded for this phase
  if (d_sharedState->isCopyDataTimestep()) {
    MPIScheduler::execute(tgnum, iteration);
    return;
  }

  ASSERTRANGE(tgnum, 0, static_cast<int>(graphs.size()));
  TaskGraph* tg = graphs[tgnum];
  tg->setIteration(iteration);
  currentTG_ = tgnum;

  if (graphs.size() > 1) {
    // tg model is the multi TG model, where each graph is going to need to
    // have its dwmap reset here (even with the same tgnum)
    tg->remapTaskDWs(dwmap);
  }

  m_detailed_tasks = tg->getDetailedTasks();

  if (m_detailed_tasks == 0) {
    proc0cout << "ThreadedScheduler skipping execute, no tasks\n";
    return;
  }

  m_detailed_tasks->initializeScrubs(dws, dwmap);
  m_detailed_tasks->initTimestep();

  m_num_tasks = m_detailed_tasks->numLocalTasks();
  for (int i = 0; i < m_num_tasks; i++) {
    m_detailed_tasks->localTask(i)->resetDependencyCounts();
  }

  int my_rank = d_myworld->myrank();
  makeTaskGraphDoc(m_detailed_tasks, my_rank);

  mpi_info_.reset( 0 );

  m_num_tasks_done = 0;
  m_abort = false;
  m_abort_point = 987654;

  if (reloc_new_posLabel_ && dws[dwmap[Task::OldDW]] != 0) {
    dws[dwmap[Task::OldDW]]->exchangeParticleQuantities(m_detailed_tasks, getLoadBalancer(), reloc_new_posLabel_, iteration);
  }

  m_current_iteration = iteration;
  m_current_phase = 0;
  m_num_phases = tg->getNumTaskPhases();
  m_phase_tasks.clear();
  m_phase_tasks.resize(m_num_phases, 0);
  m_phase_tasks_done.clear();
  m_phase_tasks_done.resize(m_num_phases, 0);
  m_phase_sync_tasks.clear();
  m_phase_sync_tasks.resize(m_num_phases, nullptr);

  // count the number of tasks in each task-phase
  //   each task is assigned a task-phase in TaskGraph::createDetailedDependencies()
  for (int i = 0; i < m_num_tasks; i++) {
    m_phase_tasks[m_detailed_tasks->localTask(i)->getTask()->d_phase]++;
  }

  if (threaded_dbg.active()) {
    std::lock_guard<std::mutex> wait_guard(s_io_mutex);
    {
      threaded_dbg << "\n" << "Rank-" << my_rank << " Executing " << m_detailed_tasks->numTasks() << " tasks (" << m_num_tasks
          << " local)\n" << "Total task phases: " << m_num_phases << "\n";
      for (size_t phase = 0; phase < m_phase_tasks.size(); ++phase) {
        threaded_dbg << "Phase: " << phase << " has " << m_phase_tasks[phase] << " total tasks\n";
      }
      threaded_dbg << "\n" << "Rank-" << my_rank << " starting task phase " << m_current_phase << ", total phase " << m_current_phase << " tasks = "
                   << m_phase_tasks[m_current_phase] << std::endl;
    }
  }


  //------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------

  // reset per-thread wait times and activate
  for (int i = 1; i < m_num_threads; i++) {
    Impl::g_runners[i]->m_task_wait_time.reset();
    Impl::g_thread_states[i] = Impl::ThreadState::Active;
  }

//  // for now, the main thread does NOT execute tasks, only processes MPI
//  select_tasks();

  Impl::thread_fence();

  for (int i = 1; i < m_num_threads; i++) {
    Impl::g_thread_states[i] = Impl::ThreadState::Inactive;
  }

  //------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------


  emitNetMPIStats();

  // compute the net timings and add in wait times for all TaskRunner threads
  if (d_sharedState != 0) {
    computeNetRunTimeStats(d_sharedState->d_runTimeStats);
    double thread_wait_time = 0.0;
    for (int i = 1; i < m_num_threads; i++) {
      thread_wait_time += Impl::g_runners[i]->m_task_wait_time.elapsed();
    }
    d_sharedState->d_runTimeStats[SimulationState::TaskWaitThreadTime] += (thread_wait_time / (m_num_threads - 1) );
  }

  // Copy the restart flag to all processors
  reduceRestartFlag(tgnum);

  finalizeTimestep();

  log.finishTimestep();

  if (!parentScheduler_) {  // only do on toplevel scheduler
    outputTimingStats("ThreadFunnledScheduler");
  }
} // end execute()


//______________________________________________________________________
//
void ThreadFunneledScheduler::select_tasks()
{
  while (m_num_tasks_done < m_num_tasks) {




    DetailedTask* readyTask = nullptr;
    DetailedTask* initTask  = nullptr;

    int processMPIs = 0;
    bool havework   = false;

    // Part 1. Check if anything this thread can do concurrently. If so, update the scheduler counters.
    m_scheduler_lock.lock();
    while (!havework) {

      //-----------------------------------------------------------------------
      // if we have a reduction task and we're on the last task in a phase... do the reduction
      if ((m_phase_sync_tasks[m_current_phase] != nullptr) && (m_phase_tasks_done[m_current_phase] == m_phase_tasks[m_current_phase] - 1)) {
        readyTask = m_phase_sync_tasks[m_current_phase];
        havework = true;
        m_num_tasks_done++;
        m_phase_tasks_done[readyTask->getTask()->d_phase]++;
        while (m_phase_tasks[m_current_phase] == m_phase_tasks_done[m_current_phase] && m_current_phase + 1 < m_num_phases) {
          m_current_phase++;

          std::lock_guard<std::mutex> wait_guard(s_io_mutex);
          {
            threaded_dbg << "Rank-" <<d_myworld->myrank() << " switched to task phase " << m_current_phase << ", total phase " << m_current_phase << " tasks = "
                         << m_phase_tasks[m_current_phase] << std::endl;
          }

        }

        break;
      }


      //-----------------------------------------------------------------------
      // get an external ready task
      else if (m_detailed_tasks->numExternalReadyTasks() > 0) {
        readyTask = m_detailed_tasks->getNextExternalReadyTask();
        if (readyTask != nullptr) {
          havework = true;
          m_num_tasks_done++;
          m_phase_tasks_done[readyTask->getTask()->d_phase]++;
          while (m_phase_tasks[m_current_phase] == m_phase_tasks_done[m_current_phase] && m_current_phase + 1 < m_num_phases) {
            m_current_phase++;


            std::lock_guard<std::mutex> wait_guard(s_io_mutex);
            {
              threaded_dbg << "Rank-" <<d_myworld->myrank() << " switched to task phase " << m_current_phase << ", total phase " << m_current_phase << " tasks = "
                           << m_phase_tasks[m_current_phase] << std::endl;
            }

          }
          break;
        }
      }


      //-----------------------------------------------------------------------
      // get an internal ready task
      else if (m_detailed_tasks->numInternalReadyTasks() > 0) {
        initTask = m_detailed_tasks->getNextInternalReadyTask();

        if (initTask != nullptr) {

          if (initTask->getTask()->getType() == Task::Reduction || initTask->getTask()->usesMPI()) {
            m_phase_sync_tasks[initTask->getTask()->d_phase] = initTask;
            ASSERT(initTask->getRequires().size() == 0) // should have NO external dependencies
            initTask = nullptr;
          }

          else if (initTask->getRequires().size() == 0) {  // if no external dependencies, skip MPI sends
            initTask->markInitiated();
            initTask->checkExternalDepCount();
            initTask = nullptr;
          }

          else {
            havework = true;
            break;
          }
        }
      }


      //-----------------------------------------------------------------------
      // we couldn't get a ready task, and will process MPI recvs below
      else {
        processMPIs = pendingMPIRecvs();
        if (processMPIs > 0) {
          havework = true;
          break;
        }
      }

      if (m_num_tasks_done == m_num_tasks) {
        break;
      }
    }
    m_scheduler_lock.unlock();



    // Part 2. Concurrent Part
    if (initTask != nullptr) {

      // TODO - FIXME: these calls should be made from the man thread
      initiateTask(initTask, m_abort, m_abort_point, m_current_iteration);
      initTask->markInitiated();
      initTask->checkExternalDepCount();


    } else if (readyTask != nullptr) {


      // TODO - FIXME: these calls should be made from the man thread
      if (readyTask->getTask()->getType() == Task::Reduction) {
        initiateReduction(readyTask);
      }


      else {
        run_task(readyTask, m_current_iteration);

        if (taskdbg.active()) {
          taskdbg << d_myworld->myrank() << " Completed task:  \t";
          printTask(taskdbg, readyTask);
          taskdbg << '\n';
        }

      }
    }


    // TODO - FIXME: these calls should be made from the man thread
    else if (processMPIs > 0) {
      processMPIRecvs(TEST);
    }


    else {
      //This could only happen when finished all tasks
      ASSERT(m_num_tasks_done == m_num_tasks);
    }
  }  //end while tasks
}


//______________________________________________________________________
//
void ThreadFunneledScheduler::run_task( DetailedTask * task , int iteration )
{
  if (waitout.active()) {
    std::lock_guard<std::mutex> wait_guard(s_io_mutex);
    {
      waittimes[task->getTask()->getName()] += g_thread_funneled_current_wait_time;
      g_thread_funneled_current_wait_time = 0;
    }
  }

  if (trackingVarsPrintLocation_ & SchedulerCommon::PRINT_BEFORE_EXEC) {
    printTrackedVars(task, SchedulerCommon::PRINT_BEFORE_EXEC);
  }

  std::vector<DataWarehouseP> plain_old_dws(dws.size());
  for (int i = 0; i < (int)dws.size(); i++) {
    plain_old_dws[i] = dws[i].get_rep();
  }


  // -------------------------< begin task execution timing >-------------------------
  Impl::g_runners[Impl::t_tid]->m_task_exec_time.reset();
  task->doit(d_myworld, dws, plain_old_dws);
  double total_task_time = Impl::g_runners[Impl::t_tid]->m_task_exec_time.elapsed();
  // -------------------------< end task execution timing >---------------------------


  if (trackingVarsPrintLocation_ & SchedulerCommon::PRINT_AFTER_EXEC) {
    printTrackedVars(task, SchedulerCommon::PRINT_AFTER_EXEC);
  }

  // TODO - FIXME - should become lock-free
  std::lock_guard<std::mutex> lb_guard(s_lb_mutex);
  {
    if (execout.active()) {
      exectimes[task->getTask()->getName()] += total_task_time;
    }

    // If I do not have a sub scheduler
    if (!task->getTask()->getHasSubScheduler()) {
      //add my task time to the total time
      // TODO - FIXME: this is wrong, shold be computed nthreads-1 separate times and then averaged (also need to be atomic) - APH (02/17/16)
      mpi_info_[TotalTask] += total_task_time;
      if (!d_sharedState->isCopyDataTimestep() && task->getTask()->getType() != Task::Output) {
        // add contribution of task execution time to load balancer
        getLoadBalancer()->addContribution(task, total_task_time);
      }
    }
  } // std::lock_guard<std::mutex> lb_guard(s_lb_mutex);


  // TODO - FIXME - these calls should be made from the man thread
  //  will also need to get rid of the TaskRunner member: m_mpi_test_time
  postMPISends(task, iteration, Impl::t_tid);
  task->done(dws);  // should this be timed with taskstart?



  // -------------------------< begin MPI test timing >-------------------------
  Impl::g_runners[Impl::t_tid]->m_mpi_test_time.reset();
  sends_[Impl::t_tid].testsome(d_myworld);
  mpi_info_[TotalTestMPI] += Impl::g_runners[Impl::t_tid]->m_mpi_test_time.elapsed();
  // -------------------------< end MPI test timing >-------------------------




  // Add subscheduler timings to the parent scheduler and reset subscheduler timings
  if (parentScheduler_) {
    for (size_t i = 0; i < mpi_info_.size(); ++i) {
      MPIScheduler::TimingStat e = (MPIScheduler::TimingStat)i;
      parentScheduler_->mpi_info_[e] += mpi_info_[e];
    }
    mpi_info_.reset(0);
  }

}  // end runTask()


//______________________________________________________________________
//
void ThreadFunneledScheduler::set_runner( TaskRunner * runner, int tid )
{
  Impl::g_runners[tid] = runner;
  std::atomic_thread_fence(std::memory_order_seq_cst);
}


//______________________________________________________________________
//
void ThreadFunneledScheduler::init_threads(ThreadFunneledScheduler * sched, int num_threads )
{
  Impl::init_threads(sched, num_threads);
}


//______________________________________________________________________
//
void
TaskRunner::run() const
{
  m_scheduler->select_tasks();
}

