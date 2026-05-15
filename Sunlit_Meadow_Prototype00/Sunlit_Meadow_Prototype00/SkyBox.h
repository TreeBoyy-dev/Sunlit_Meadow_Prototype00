#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <vector>
#include "Vectors.h"
#include "DataStructures.h"

#define FACE_LENGTH 512

class Skybox {
private:
	SDL_GPUTexture* cubemap;

	SDL_GPUGraphicsPipeline* pipeline;
	SDL_GPUBuffer*			 vertexBuffer;
	SDL_GPUSampler*		     sampler;
	std::vector<Vec3>		 verts;

	SDL_GPUTexture* LoadCubemap(
		AppState* state,
		const char* filePath,
		const char* fileName);
public:
	Skybox();

	bool init(
		AppState* state,
		SDL_GPUTextureFormat swapchainFormat,
		const char* filePath,
		const char* fileName
	);
	void destroy(SDL_GPUDevice* gpu);

	void upload(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd);
	void draw(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd, const UBO& ubo);
};