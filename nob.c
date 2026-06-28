#define NOB_IMPLEMENTATION
#include "nob.h"
#define BIN_DIR "bin/"
#define BUILD_DIR "build/"
#define SRC_DIR "src/"
#define RSOURCE "vendor/raylib/src/"
#define GLFWIN "-I./vendor/raylib/src/external/glfw/include"
#define GAME_NAME "jam_thing"
#define OPENGLES_VER "-DGRAPHICS_API_OPENGL_ES2"

bool web = false;
bool nobuild = false;
bool run = false;
const char *rayfiles[] = {
	"rglfw", "raudio", "rcore", "rmodels", "rshapes", "rtext", "rtextures",
};

bool build_raylib_linux(Nob_Cmd *cmd)
{
	Nob_File_Paths object_files = { 0 };
	Nob_Procs procs = { 0 };
	char *build_path = BUILD_DIR "raylib_linux/";
	if (!nob_mkdir_if_not_exists(build_path))
		return false;

	for (size_t i = 0; i < NOB_ARRAY_LEN(rayfiles); ++i) {
		const char *input_path =
			nob_temp_sprintf(RSOURCE "%s.c", rayfiles[i]);
		const char *output_path =
			nob_temp_sprintf("%s%s.o", build_path, rayfiles[i]);

		nob_da_append(&object_files, output_path);

		if (nob_needs_rebuild(output_path, &input_path, 1)) {
			nob_cmd_append(cmd, "cc");
			nob_cmd_append(cmd, "-std=gnu99");
			nob_cmd_append(cmd, "-ggdb");
			nob_cmd_append(cmd, "-O2");
			nob_cmd_append(cmd, "-DPLATFORM_DESKTOP");
			nob_cmd_append(cmd, "-D_GLFW_X11");
			nob_cmd_append(cmd, "-I./raylib/src/");
			nob_cmd_append(cmd, GLFWIN);
			nob_cmd_append(cmd, "-c");
			nob_cmd_append(cmd, input_path);
			nob_cmd_append(cmd, "-o");
			nob_cmd_append(cmd, output_path);
			if (!nob_cmd_run(cmd, .async = &procs))
				return false;
		}
	}

	nob_temp_reset();
	if (!nob_procs_wait(procs))
		return false;

	cmd_append(cmd, "ar");
	cmd_append(cmd, "-crs");
	cmd_append(cmd, BUILD_DIR "raylib_linux/libraylib.a");
	for (int i = 0; i < NOB_ARRAY_LEN(rayfiles); i++) {
		const char *objfile = nob_temp_sprintf(
			"%s%s.o", BUILD_DIR "raylib_linux/", rayfiles[i]);
		nob_cmd_append(cmd, objfile);
	}
	nob_temp_reset();
	if (!nob_cmd_run(cmd))
		return false;

	return true;
}

bool build_raylib_macos(Nob_Cmd *cmd)
{
	Nob_File_Paths object_files = { 0 };
	Nob_Procs procs = { 0 };
	char *build_path = BUILD_DIR "raylib_macos/";
	if (!nob_mkdir_if_not_exists(build_path))
		return false;

	for (size_t i = 0; i < NOB_ARRAY_LEN(rayfiles); ++i) {
		const char *input_path =
			nob_temp_sprintf(RSOURCE "%s.c", rayfiles[i]);
		const char *output_path =
			nob_temp_sprintf("%s%s.o", build_path, rayfiles[i]);

		nob_da_append(&object_files, output_path);

		if (nob_needs_rebuild(output_path, &input_path, 1)) {
			nob_cmd_append(cmd, "cc");
			nob_cmd_append(cmd, "-std=c99");
			nob_cmd_append(cmd, "-g");
			nob_cmd_append(cmd, "-O2");
			nob_cmd_append(cmd, "-DPLATFORM_DESKTOP");
			nob_cmd_append(cmd, "-D_GLFW_COCOA");
			nob_cmd_append(cmd, "-I./raylib/src/");
			nob_cmd_append(cmd, GLFWIN);
			nob_cmd_append(cmd, "-c");
			nob_cmd_append(cmd, input_path);
			nob_cmd_append(cmd, "-o");
			nob_cmd_append(cmd, output_path);
			if (!nob_cmd_run(cmd, .async = &procs))
				return false;
		}
	}

	nob_temp_reset();
	if (!nob_procs_wait(procs))
		return false;

	cmd_append(cmd, "ar");
	cmd_append(cmd, "-crs");
	cmd_append(cmd, BUILD_DIR "raylib_macos/libraylib.a");
	for (int i = 0; i < NOB_ARRAY_LEN(rayfiles); i++) {
		const char *objfile = nob_temp_sprintf(
			"%s%s.o", BUILD_DIR "raylib_macos/", rayfiles[i]);
		nob_cmd_append(cmd, objfile);
	}
	nob_temp_reset();
	if (!nob_cmd_run(cmd))
		return false;

	return true;
}

bool build_raylib_web(Nob_Cmd *cmd)
{
	Nob_File_Paths object_files = { 0 };
	Nob_Procs procs = { 0 };
	char *build_path = BUILD_DIR "raylib_web/";
	if (!nob_mkdir_if_not_exists(build_path))
		return false;

	// NOTE: start at 1 to skip rglfw
	for (size_t i = 1; i < NOB_ARRAY_LEN(rayfiles); ++i) {
		const char *input_path =
			nob_temp_sprintf(RSOURCE "%s.c", rayfiles[i]);
		const char *output_path =
			nob_temp_sprintf("%s%s.o", build_path, rayfiles[i]);

		nob_da_append(&object_files, output_path);

		if (nob_needs_rebuild(output_path, &input_path, 1)) {
			nob_cmd_append(cmd, "emcc");
			nob_cmd_append(cmd, "-std=gnu99");
			nob_cmd_append(cmd, "-Os");
			nob_cmd_append(cmd, "-DPLATFORM_WEB");
			nob_cmd_append(cmd, OPENGLES_VER);
			nob_cmd_append(cmd, "-I./raylib/src/");
			nob_cmd_append(cmd, GLFWIN);
			nob_cmd_append(cmd, "-c");
			nob_cmd_append(cmd, input_path);
			nob_cmd_append(cmd, "-o");
			nob_cmd_append(cmd, output_path);
			// nob_cmd_append(cmd, "--profiling");
			cmd_append(cmd, "-s", "USE_GLFW=3");
			if (!nob_cmd_run(cmd, .async = &procs))
				return false;
		}
	}

	nob_temp_reset();
	if (!nob_procs_wait(procs))
		return false;

	cmd_append(cmd, "emar");
	cmd_append(cmd, "-crs");
	cmd_append(cmd, BUILD_DIR "raylib_web/libraylib.a");

	// NOTE: start at 1 to skip rglfw
	for (int i = 1; i < NOB_ARRAY_LEN(rayfiles); i++) {
		const char *objfile = nob_temp_sprintf(
			"%s%s.o", BUILD_DIR "raylib_web/", rayfiles[i]);
		nob_cmd_append(cmd, objfile);
	}
	nob_temp_reset();
	if (!nob_cmd_run(cmd))
		return false;

	return true;
}

bool build_raylib_msvc(Nob_Cmd *cmd)
{
	Nob_File_Paths object_files = { 0 };
	Nob_Procs procs = { 0 };
	char *build_path = BUILD_DIR "raylib_msvc/";
	if (!nob_mkdir_if_not_exists(build_path))
		return false;

	for (size_t i = 0; i < NOB_ARRAY_LEN(rayfiles); ++i) {
		const char *input_path =
			nob_temp_sprintf(RSOURCE "%s.c", rayfiles[i]);
		const char *output_path =
			nob_temp_sprintf("%s%s.obj", build_path, rayfiles[i]);

		nob_da_append(&object_files, output_path);

		if (nob_needs_rebuild(output_path, &input_path, 1)) {
			nob_cmd_append(cmd, "cl");
			cmd_append(cmd, "/nologo");
			nob_cmd_append(cmd, "/c");
			nob_cmd_append(cmd, "/O2");
			nob_cmd_append(cmd, "-DPLATFORM_DESKTOP");
			nob_cmd_append(cmd, "/Iraylib/src/");
			nob_cmd_append(cmd, GLFWIN);
			nob_cmd_append(cmd, input_path);
			nob_cmd_append(cmd, "/Fo" BUILD_DIR "raylib_msvc/");
			if (!nob_cmd_run(cmd, .async = &procs))
				return false;
		}
	}

	nob_temp_reset();
	if (!nob_procs_wait(procs))
		return false;

	cmd_append(cmd, "lib");
	cmd_append(cmd, "/nologo");
	cmd_append(cmd, "/OUT:" BUILD_DIR "raylib_msvc/raylib.lib");
	for (int i = 0; i < NOB_ARRAY_LEN(rayfiles); i++) {
		const char *objfile = nob_temp_sprintf(
			"%s%s.obj", BUILD_DIR "raylib_msvc/", rayfiles[i]);
		nob_cmd_append(cmd, objfile);
	}
	nob_temp_reset();
	if (!nob_cmd_run(cmd))
		return false;

	return true;
}

void run_web(Cmd cmd)
{
	cmd_append(&cmd, "python3");
	cmd_append(&cmd, "-m");
	cmd_append(&cmd, "http.server");
	cmd_append(&cmd, "8080");
	cmd_append(&cmd, "-d");
	cmd_append(&cmd, BIN_DIR);
	if (!cmd_run(&cmd))
		exit(1);
}

bool build_linux(Nob_Cmd *cmd)
{
	if (!nob_file_exists(BUILD_DIR "raylib_linux/libraylib.a")) {
		if (!build_raylib_linux(cmd))
			return false;
	}
	nob_cc(cmd);
	nob_cc_flags(cmd);
	nob_cmd_append(cmd, "-ggdb");
	nob_cc_inputs(cmd, SRC_DIR "unity.c");
	nob_cc_output(cmd, BIN_DIR GAME_NAME);
	nob_cmd_append(cmd, "-I./" RSOURCE);
	cmd_append(cmd, "-Ivendor");
	nob_cmd_append(cmd, BUILD_DIR "raylib_linux/libraylib.a");

	nob_cmd_append(cmd, "-lm");
	nob_cmd_append(cmd, "-lX11");

	if (!nob_cmd_run(cmd))
		return false;

	return true;
}

bool build_macos(Nob_Cmd *cmd)
{
	if (!nob_file_exists(BUILD_DIR "raylib_macos/libraylib.a")) {
		if (!build_raylib_macos(cmd))
			return false;
	}
	nob_cmd_append(cmd, "cc");
	nob_cmd_append(cmd, "-std=c99");
	nob_cmd_append(cmd, "-g");
	nob_cmd_append(cmd, "-O2");
	nob_cmd_append(cmd, "-DPLATFORM_DESKTOP");
	nob_cmd_append(cmd, SRC_DIR "unity.c");
	nob_cmd_append(cmd, "-o");
	nob_cmd_append(cmd, BIN_DIR GAME_NAME);
	nob_cmd_append(cmd, "-I./" RSOURCE);
	cmd_append(cmd, "-Ivendor");
	nob_cmd_append(cmd, BUILD_DIR "raylib_macos/libraylib.a");

	nob_cmd_append(cmd, "-framework", "CoreVideo");
	nob_cmd_append(cmd, "-framework", "IOKit");
	nob_cmd_append(cmd, "-framework", "Cocoa");
	nob_cmd_append(cmd, "-framework", "GLUT");
	nob_cmd_append(cmd, "-framework", "OpenGL");

	if (!nob_cmd_run(cmd))
		return false;

	return true;
}

bool build_web(Nob_Cmd *cmd)
{
	if (!nob_file_exists(BUILD_DIR "raylib_web/libraylib.a")) {
		if (!build_raylib_web(cmd))
			return false;
	}
	cmd_append(cmd, "emcc");
	cmd_append(cmd, "-o");
	cmd_append(cmd, BIN_DIR "index.html");
	cmd_append(cmd, SRC_DIR "unity.c");
	cmd_append(cmd, "-Os");
	cmd_append(cmd, "-Wall");
	cmd_append(cmd, "-DPLATFORM_WEB");
	cmd_append(cmd, OPENGLES_VER);
	cmd_append(cmd, "--shell-file");
	cmd_append(cmd, SRC_DIR "shell.html");
	cmd_append(cmd, BUILD_DIR "raylib_web/libraylib.a");
  cmd_append(cmd, "--embed-file", "assets");
	cmd_append(cmd, "-I" RSOURCE);
	cmd_append(cmd, "-I" SRC_DIR);
	cmd_append(cmd, "-Ivendor");
	cmd_append(cmd, "-s", "USE_GLFW=3");
  cmd_append(cmd, "-s", "ASSERTIONS");
  cmd_append(cmd, "-s", "MAXIMUM_MEMORY=2GB");
  cmd_append(cmd, "-s", "ALLOW_MEMORY_GROWTH=1");
#ifdef ASYNCIFY
	cmd_append(cmd, "-s");
	cmd_append(cmd, "ASYNCIFY");
#endif
	if (!cmd_run(cmd))
		return false;
	return true;
}

bool package_web(Nob_Cmd *cmd) {
  cmd_append(cmd, "zip");
  cmd_append(cmd, "-r", BIN_DIR"pizza-panic.zip");
  cmd_append(cmd, BIN_DIR"index.html");
  cmd_append(cmd, BIN_DIR"index.js");
  cmd_append(cmd, BIN_DIR"index.wasm");
	if (!cmd_run(cmd))
		return false;
  return true;
}

bool build_msvc(Nob_Cmd *cmd)
{
	if (!nob_file_exists(BUILD_DIR "raylib_msvc/raylib.lib")) {
		if (!build_raylib_msvc(cmd))
			return false;
	}
	cmd_append(cmd, "cl");
	cmd_append(cmd, "/nologo");
	nob_cmd_append(cmd, "/W3");
	nob_cc_inputs(cmd, SRC_DIR "unity.c");
	nob_cc_output(cmd, BIN_DIR GAME_NAME);
	nob_cmd_append(cmd, "-I./" RSOURCE);
	cmd_append(cmd, "-Ivendor");
	nob_cmd_append(cmd, BUILD_DIR "raylib_msvc/raylib.lib");
	cmd_append(cmd, "opengl32.lib");
	cmd_append(cmd, "gdi32.lib");
	cmd_append(cmd, "user32.lib");
	cmd_append(cmd, "shell32.lib");
	cmd_append(cmd, "winmm.lib");
	cmd_append(cmd, "comdlg32.lib");
	cmd_append(cmd, "ole32.lib");

	if (!nob_cmd_run(cmd))
		return false;

	return true;
}

bool build()
{
	if (!nob_mkdir_if_not_exists(BIN_DIR))
		return false;
	if (!nob_mkdir_if_not_exists(BUILD_DIR))
		return false;
	if (!nob_copy_directory_recursively("assets", BIN_DIR "assets"))
		return false;
	Nob_Cmd cmd = { 0 };
#ifdef _MSC_VER
	if (!build_msvc(&cmd))
		return false;
#elif defined(__APPLE__)
	if (!build_macos(&cmd))
		return false;
#else
	if (web) {
		if (!build_web(&cmd))
			return false;
    if (!package_web(&cmd))
      return false;
	} else {
		if (!build_linux(&cmd))
			return false;
	}
#endif //_MSC_VER

	return true;
}

int main(int argc, char **argv)
{
	NOB_GO_REBUILD_URSELF(argc, argv);

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "nobuild") == 0)
			nobuild = true;
		else if (strcmp(argv[i], "web") == 0)
			web = true;
		else if (strcmp(argv[i], "run") == 0)
			run = true;
		else {
			printf("Unknown arg: %s\n", argv[i]);
			exit(1);
		}
	}
	if (!nobuild)
		if (!build())
			exit(1);

	if (run) {
		Cmd cmd = { 0 };
		if (web) {
			cmd_append(&cmd, "python3");
			cmd_append(&cmd, "-m");
			cmd_append(&cmd, "http.server");
			cmd_append(&cmd, "8080");
			cmd_append(&cmd, "-d");
			cmd_append(&cmd, BIN_DIR);
			if (!cmd_run(&cmd))
				exit(1);
		} else {
			const char *exe_name =
#ifdef _WIN32
				BIN_DIR GAME_NAME ".exe";
#else
				BIN_DIR GAME_NAME;
#endif //_WIN32

			cmd_append(&cmd, exe_name);
			if (!cmd_run(&cmd))
				exit(1);
		}
	}

	return 0;
}
