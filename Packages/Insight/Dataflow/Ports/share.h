#undef SCISHARE

#ifdef _WIN32
#  ifdef BUILD_Packages_Insight_Dataflow_Ports
#    define SCISHARE __declspec(dllexport)
#  else
#    define SCISHARE __declspec(dllimport)
#  endif
#else
#  define SCISHARE
#endif
