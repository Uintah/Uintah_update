#undef SHARE

#ifdef _WIN32
#ifdef BUILD_Dataflow_Modules_Fields
#define SHARE __declspec(dllexport)
#else
#define SHARE __declspec(dllimport)
#endif
#else
#define SHARE
#endif
