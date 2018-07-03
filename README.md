# The IYFEngine
## The Mythos
> 'twas an evening of late September.  A downpour was thoroughly drenching me and a tempestuous wind reflected my inner turmoil quite well. Yet another startup I was a part of had flopped and months of coding went down the drain. I was wandering around town, trying to clear my head and I suddenly came to an unpleasant realization. I realized I had once again lost myself to work and I desperately needed to do something different; something personal and, most importantly, fun. And few things are more fun than practising necromancy... *of the coding kind*.
>
>I exhumed its corpse from an archival HDD. It was a tiny OpenGL based framework. I wrote it during my second year of university, but it never saw much use. In the grand scheme of things, it was too simple, poorly structured and unsuitable for any significant game development, but I felt it had... potential. I had a hunch that, eventually, I'll be able to weave its code-flesh into something beautiful, modern and, most importantly, **version controlled**.
>
>At the moment, it's still far from being usable in game development, but I decided to finally put it on GitHub as a showcase of my skills and as a vector of collaboration with fellow code-flesh weavers. So come, I invite you to join me in this endeavour, but be careful, for the price of dabbling in necromancy (of the coding kind or not) is... _**your SOUL**_.

**Nah.** Just kidding. It's licensed under 3-Clause BSD. Your free time will suffer, though.

## So, what is it?
The IYFEngine is my pet project. I want to create a fully-featured open source engine for desktop (Windows, Linux, Mac) and mobile (Android and iOS) platforms. I want to focus on performance and the use of up-to-date technologies or standards, such as Vulkan and C++17.

## A word of caution
The IYFEngine is in a **very incomplete state** at the moment. Many systems are work in progress or haven't even been started yet. Moreover, while I will make the engine cross platform eventually, at the moment it requires some Linux specific libraries to compile (e.g., inotify that is used to monitor the project directory for newly added or changed asset files). Last but not least, my CMake files were made to be used with GCC or Clang (some compilation and linking options are hard-coded) and will break if you try to generate build scripts for Visual Studio.

I do my best to immediately fix (**NOT SILENCE**) most warnings that I come across. The CMake scripts set `-Wall`, `-Wextra` and `-pedantic` compilation flags. However, you will still see warnings about unused parameters from methods that I haven't implemented yet. They're kept to remind me there's still work to do in specific classes. Any other warnings should be reported immediately.

## Building the engine
0. Clone the code stored in this repository.
1. Open the DEPENDENCIES.md file and download/install all listed dependencies.
2. Create the *build* folder in the root directory (where this README.md file is located). The name *build* is important because it is included in *.gitignore* and it's also used in some system asset packaging tools. Use this:

  ```bash
  mkdir build && cd build
  ```

3. Run make:

  ```bash
  make -j8
  ```
4. Convert and pack the system assets. Run the SystemAssetPacker executable that was created in the *build* folder:

  ```bash
  ./SystemAssetPacker
  ```

5. That's it. You can now start the editor:

  ```bash
  ./IYFEditor
  ```

## FAQs
1. **Where's the pre-GitHub commit history? Why did you remove it?**

    While I would have liked to keep it, I was, unfortunately, forced to purge it. Turns out, I was using certain bits of code that I *"didn't have the right to use"*. Long story short, make sure to **always get everything in writing**. If you have a falling out, some people may not want to keep their word.

    The said code was removed more than a year ago before the first public commit. The functionality in question was rewritten and is even better now. However, the history had to go to prevent it from being recovered.
