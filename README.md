# Shadow and Skully - C++ Code
2019 Summer Project - UE4 4.22

### Background
This is just the C++ code for the summer project that I worked on, Shadow and Skully.
This project was developed over a 9-week timeframe and it was the first project that I was able to work full-time (40+ hours/week)

Before this, I had only worked in Unreal for appx. 3 months, so there was still a learning curve on my part, as there is still much more that I want to learn.

I was the only developer messing with the cpp-side, so my code was not reviewed or worked on by anyone else. I still attempted to keep it clean for readability, but that had the tendancy to go out the window during crunch time.

This was my first time using LFS for Unreal, so there were quite a few issues that were created due to my inexperience that I had to solve.

### What I'm Proud of
1. This is my first real time understanding UE4's structure, so I feel that it shows with some pretty good code. I tried to also incorporate the needs of the team by creating variables for them to tweak in the editor. I also tried to hide some of the less-critical variables to prevent them from being overwhelming.

2. I am personally proud of the Skully AI. I think it's the feature I spent the most time on, and I think it shows. There are certainly a lot of things I want to/will change, but it turned out fairly well.

3. I like all the small tools I added to try and make implementation of features easier. Specifically, I drew an arrow from the puzzle activator to the puzzle activatee on selection of either (ie lever->door) to help ease the development of puzzles. 

### What I'd Change
1. As nice as some of it turned out, there are also parts of my code that I didn't exactly organize or comment well.
2. There are quite a few outdated or redundant systems that only add to the ineffeciency of the game, so I would have spent more time optomizing better.
3. A lot of the systems aren't as complex as I feel would have been necessary for a full game. The AI specifically didn't react to environmental changes as well as I would've liked.
4. I threw a lot of decisions on the Tick function, but I would tried to remove as much functionality from the main thread as possible.
5. I would have made better use of namespaces to clean up the PuzzleElement file. I threw too much in there and should have seperated it better.
6. The async function in the dynamic camera is horrible. I just have it constantly running, which is a waste of resources. I would have absolutely fixed that.
