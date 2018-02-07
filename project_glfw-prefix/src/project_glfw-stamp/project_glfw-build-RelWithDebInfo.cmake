

set(command "C:/Users/Aden/Desktop/cmake-3.10.2-win64-x64/bin/cmake.exe;--build;.;--config;RelWithDebInfo")
execute_process(
  COMMAND ${command}
  RESULT_VARIABLE result
  OUTPUT_FILE "C:/A_PCL/RenderEngine/Horde3D/project_glfw-prefix/src/project_glfw-stamp/project_glfw-build-out.log"
  ERROR_FILE "C:/A_PCL/RenderEngine/Horde3D/project_glfw-prefix/src/project_glfw-stamp/project_glfw-build-err.log"
  )
if(result)
  set(msg "Command failed: ${result}\n")
  foreach(arg IN LISTS command)
    set(msg "${msg} '${arg}'")
  endforeach()
  set(msg "${msg}\nSee also\n  C:/A_PCL/RenderEngine/Horde3D/project_glfw-prefix/src/project_glfw-stamp/project_glfw-build-*.log")
  message(FATAL_ERROR "${msg}")
else()
  set(msg "project_glfw build command succeeded.  See also C:/A_PCL/RenderEngine/Horde3D/project_glfw-prefix/src/project_glfw-stamp/project_glfw-build-*.log")
  message(STATUS "${msg}")
endif()
