
echo Compiling shaders...
shadercross.exe shader.vert.hlsl -o shader.vert.spv
if %errorlevel% neq 0 ( echo Vertex shader failed. & exit /b 1 )

shadercross.exe shader.frag.hlsl -o shader.frag.spv
if %errorlevel% neq 0 ( echo Fragment shader failed. & exit /b 1 )

shadercross.exe ui.vert.hlsl -o ui.vert.spv
if %errorlevel% neq 0 ( echo Vertex ui shader failed. & exit /b 1 )

shadercross.exe ui.frag.hlsl -o ui.frag.spv
if %errorlevel% neq 0 ( echo Fragment ui shader failed. & exit /b 1 )

shadercross.exe ui_tex.vert.hlsl -o ui_tex.vert.spv
if %errorlevel% neq 0 ( echo Vertex ui_tex shader failed. & exit /b 1 )

shadercross.exe skybox.frag.hlsl -o skybox.frag.spv
if %errorlevel% neq 0 ( echo Fragment skybox shader failed. & exit /b 1 )

shadercross.exe skybox.vert.hlsl -o skybox.vert.spv
if %errorlevel% neq 0 ( echo Vertex skybox shader failed. & exit /b 1 )

shadercross.exe skybox.frag.hlsl -o skybox.frag.spv
if %errorlevel% neq 0 ( echo Fragment skybox shader failed. & exit /b 1 )

shadercross.exe entity.vert.hlsl -o entity.vert.spv
if %errorlevel% neq 0 ( echo Vertex entity shader failed. & exit /b 1 )
 
shadercross.exe entity.frag.hlsl -o entity.frag.spv
if %errorlevel% neq 0 ( echo Fragment entity shader failed. & exit /b 1 )
 
echo Done.
