# GPU Programming for Scientific Uses

> This is the repository dedicated for this project related to the above mentioned course.

## Project: Equalizing a Picture's Histogramm

> The point is to make a program that does picture equalizations with parallel processing. It gets a picture, converts it to greyscale, equalizes the histogramm and outputs the equalized picture.

### Results
    
    basic(debug) /w test_pic(30x30) : ~0.7 msec

    cpu-multithread(debug) /w test_pic(30x30) : ~1.1 msec

    basic(release) /w big_pic(445x445) : ~0.9 msec

    cpu-multithread(release) /w big_pic(445x445) : ~1.6 msec

> As it seems, the test_pic is too small to use it as a refenerence. A bigger picture would make this multithread process to be better.

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

### 2021/06/21: cpu multi update.

> With tips being given from my lecturer, I was able to cut down the runtime to 1-1.1msec. The problem was that I forgot that rather than the names of the object, a reference should be given to funtions. (the summer() didn't have it) This caused a constant copying happening.
> The issue with i+1 was located. The problem was that i!=i2 causes i2 to be not included in this interval ( [i1,i2) -> [i1,i2] ).

## The test_pic.txt

### Histograms

![Test Picture Histogram](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/test_pic_hist.jpeg)

![Equaized Picture Histogram](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/eq_pic_hist.jpeg)

### Pictures

![Test pic](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/test_pic.jpeg)

![Equalized pic](https://github.com/AdamGTaylor/GPU-2020-2021-2/blob/master/_notebooks/pics_preview/eq_pic.jpeg)