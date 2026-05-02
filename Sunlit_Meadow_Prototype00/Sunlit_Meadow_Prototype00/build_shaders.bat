
echo Compiling shaders...
shadercross.exe shader.vert.hlsl -o shader.vert.spv
if %errorlevel% neq 0 ( echo Vertex shader failed. & exit /b 1 )

shadercross.exe shader.frag.hlsl -o shader.frag.spv
if %errorlevel% neq 0 ( echo Fragment shader failed. & exit /b 1 )

echo Done.
