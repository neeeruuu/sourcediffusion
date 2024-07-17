<h1 align="center">
	<!-- <img src="resources/logo.png" alt="icon" width="400" height="400"><br/> -->
    <b>SourceDiffusion</b>
</h1>
<p align="center">
	"Real-time" Stable Diffusion generation for Source
    <br/> <br/>
	<a href="https://discord.gg/uEvGbYZT9x"><b>Join the Discord server</b></a>
</p>

> [!WARNING]  
> * SourceDiffusion is in its really early stage of development, expect changes, crashes and other issues.
> * Using the wrong settings, incorrect models / VAEs / Loras can result in slow generation, poor quality images, and even crashes.
> * SDXL's default VAE has a NaN issue on fp16 (which is what ggml_conv_2d uses), so please use [this VAE](https://huggingface.co/madebyollin/sdxl-vae-fp16-fix/blob/main/sdxl_vae.safetensors) for SDXL models

## Requirements
* A really good Nvidia GPU (for now)
* A Source game (Garry's Mod only for now)
* A Stable Diffusion model 
* [this VAE](https://huggingface.co/madebyollin/sdxl-vae-fp16-fix/blob/main/sdxl_vae.safetensors) if using SDXL models

## Usage
1. Run Source Diffusion.exe and wait for the game to load
2. Press insert to open the menu
2. Select a model and vae (taesd, cnet and lora if neccessary too)
3. Click Load model and wait til state becomes Idle
4. Close the menu and load a level.
5. Open the menu and check Enabled in general

## Known issues
* taesd makes results blurry and incoherent
* invalid resolutions cause crashes

## Tested models
* [DMD2](https://huggingface.co/tianweiy/DMD2/tree/main)
* [DreamShaper XL Turbo](https://huggingface.co/Lykon/dreamshaper-xl-turbo/tree/main)
* [Realities Edge XL](https://civitai.com/models/129666?modelVersionId=356472)

## To-Do:
* [x] Fix CUDA not being used on GitHub's build
* [x] Don't generate while typing in res
* [ ] Add way to interrupt load / generation 
* [X] Fix HUD size being affected when changing CViewSetup size
* [ ] Implement other backends (CPU, ROCm, etc) 
* [ ] Improve VAE performance
* [ ] Implement function signatures for other source games
* [ ] Improve CUDA install time for build action (25m+ per build atm :<)
* [ ] Implement SD on NCNN / ONNX
 
## Contributing
If you'd like to contribute, please follow these guidelines:
1. ***Conventional commits*** - Follow the [conventional commits](https://www.conventionalcommits.org/en/v1.0.0/) specification for commit messages. This helps automate versioning and generate meaningful changelogs.
2. ***Branch Naming Convention*** - Use descriptive branch names that reflect the purpose of your changes. Preferably, use kebab-case or snake_case for branch names.
3. ***Code Style and Formatting*** - Follow the project's existing code style and formatting guidelines. Ensure .clang-format has formatted your code before commiting.
4. ***Committing Changes*** - Avoid committing unrelated changes in the same commit, Keep each commit focused on a single logical change and Squash related commits before opening a pull request to keep the commit history clean and concise.
5. ***Pull Requests*** - Please provide a clear and detailed description of the changes introduced by the pull request.

If you have any questions or need clarification on any of the guidelines, feel free to ask on the [Discord server](https://discord.gg/uEvGbYZT9x).

## Build requirements
* CUDA Toolkit
* CMake 3.29.2+
* Visual Studio
* VSCode (not needed, but recommended)