# GPU Programming for Scientific Uses

> This is the repository dedicated for this project related to the above mentioned course.

## Project: Equalizing a Picture's Histogramm

> The point is to make a program that does picture equalizations with parallel processing. It gets a picture, converts it to greyscale, equalizes the histogramm and outputs the equalized picture.

> Read about histogramm equalization here.

## Roadmap.

    1/First i will do  basic, draftlike program, that has zero parallelization. The point is to get the correct approach

    2/Secondly, I will do a CPU parallelized version of that.

    3/Thirdly, I will do a GPU parallelized version of the original code. 

## Main Points:

> 1. The picture has to be in ome format, but my program will assume grayscale

> 2. I have to evaluate the histogramm, finding its lowest value.

> 3. When I recreate the picture, should i use an vector to get the value (computing i once) Or should I count at each pixel? (The former sounds better)

> 4. Save the pic.

## Pushes

### 2021/06/19: Re-init

> This repository got initialized at the wrong place, with the wrong git ignore. This was resolved by a re-init. (.exe and other files got uploaded) After this, the work will be copied here, finished, probably in time.

### 2021/06/20: Basic, Almost

> Due the issue of wrong initialistion, i am taking  the time to fix the code, where it can be fixed. Still fixing but the main feature are in. Realised the true power of atomic.

### 2021/06/20: basic done, chrono added, cpu-parallel first thoughts

> Basic is finished with average of ~0.7 msec runtime on the small (30x30) test pic. CPU multithread version has the first thoughts in it and it is initialized.