# GPU Programming for Scientific Uses

> This is the repository dedicated for this project related to the above mentioned course.

## Project: Equalizing a Picture's Histogramm

> The point is to make a program that does picture equalizations with parallel processing. It gets a picture, converts it to greyscale, equalizes the histogramm and outputs the equalized picture.

### Results
    
    basic /w test_pic : ~0.7 msec

    cpu-multithread /w test_pic : ~2.0 msec

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

### 2021/06/20: cpu multi thread: done!

> Suprisingly its slower for the test pic, compared to what is expected. The possible reason for this is that a low of "auto"s are used due to uncertainty of the returned object time. 

## The test_pic.txt

![Test Picture Histogram](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/test_pic_hist.jpeg)

![Equaized Picture Histogram](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/eq_pic_hist.jpeg)

![Test pic](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/test_pic.jpeg)

![Equalized pic](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/eq_pic.jpeg)