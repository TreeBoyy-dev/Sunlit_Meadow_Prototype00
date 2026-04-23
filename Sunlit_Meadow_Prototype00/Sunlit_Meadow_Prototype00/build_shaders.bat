
echo Compiling shaders...
glslc shader.glsl.vert -o shader.spv.vert
if %errorlevel% neq 0 ( echo Vertex shader failed. & exit /b 1 )

glslc shader.glsl.frag -o shader.spv.frag
if %errorlevel% neq 0 ( echo Fragment shader failed. & exit /b 1 )

echo Done.
