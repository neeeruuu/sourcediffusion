<h1 align="center">
	<!-- <img src="resources/logo.png" alt="icon" width="400" height="400"><br/> -->
    <b>SourceDiffusion</b>
</h1>
<p align="center">
	"Real-time" Stable Diffusion interference for Source
    <br/> <br/>
	<a href="https://discord.gg/uEvGbYZT9x"><b>Join the Discord server</b></a>
</p>

> [!WARNING]  
> Using the wrong settings, incorrect models / VAEs / Loras can result in slow / bad images, and even crashes.

## Requirements
* An NVIDIA GPU (for now)
* A build of x64 Garry's Mod (for now)

## Installation 
1. Grab the latest release from this repo.
2. Extract it.
3. Run Source Diffusion.exe

## To-Do:
* [ ] Add way to interrupt load / generation 
* [ ] Fix HUD size being affected when changing CViewSetup size
* [ ] Implement other backends (CPU, ROCm, etc) 
* [ ] Improve VAE performance
* [ ] Implement function signatures for other source games
* [ ] Improve CUDA install time for build action (25m+ per build atm :<)
 
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