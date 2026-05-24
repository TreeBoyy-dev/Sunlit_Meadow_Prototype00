#include "BuildAbsolutePath.h"

std::string BuildAbsolutePath(
    const char* filePath,
    const char* fileName
)
{
    const char* basePath = SDL_GetBasePath();
    if (!basePath) {
        SDL_Log("SDL_GetBasePath failed: %s", SDL_GetError());
        return "";
    }

    std::string fullPath = std::string(basePath);

    if (filePath && filePath[0] != '\0') {
        fullPath += filePath;

        if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += "/";
        }
    }

    fullPath += fileName;
    return fullPath;
}
