#include <iostream>
#include <cmath>
#include <fstream>

#include <vector>
#include <random>
#include <numeric>
#include <future>

int size1 = 0;
int size2 = 0;

static const int histo_size = 256;

//this is a very good struct!
template <typename T>
struct atomic_wrapper
{
  std::atomic<T> data;

  atomic_wrapper():data(){}
  atomic_wrapper(std::atomic<T> const& a   ):data(a.load()){}
  atomic_wrapper(atomic_wrapper const& copy):data(copy.data.load()){}
  atomic_wrapper& operator=(atomic_wrapper const& copy){ data.store(copy.data.load()); return *this; }
};

template<typename T>
void save_pic(std::vector<T> veced_pic){
    int count = 0;
    std::ofstream my_file("../../../output/test_pic.txt");
    my_file << size1 << " " << size2 << "\n";
    for(int i=0; i!=size1; ++i){
        for(int j=0;j!=size2-1; ++j){
            my_file << veced_pic[i*size1 + j] << " ";
            count++;
        }
        my_file << veced_pic[i*size1 + size2 -1] << "\n";
        count++;
    }
}

int main(int, char**) {
    //main thing
    std::cout << "Basic Histogramm Equalizer Started.\n";
    //init
    std::vector<int> pic;               //input pic
    std::vector<int> eq_pic;            //output pic

    std::vector<int> histogram(256,0);  //his of pic
    std::vector<int> cdf(256,0);        //point mass function of pic
    std::vector<int> h_v(256,0);        //new values for eq_pic created from eq

    //NO NEED FOR THREADS NOW
    //int max_num_of_threads = (int)std::thread::hardware_concurrency();
	//std::cout << "Using " << max_num_of_threads << " threads.\n";

    //loading in texted picture
    std::ifstream myfile("../../../pics/test_pic.txt");
    if ( myfile.is_open() ){
        myfile >> size1;
        myfile >> size2;
        pic.resize(size1*size2);
        eq_pic.resize(size1*size2);
        for(int i=0; i < size1 * size2; ++i){
            myfile >> pic[i];
        }
        std::cout << "Succesful loading" << std::endl;
    }
    
    auto time0 = std::chrono::high_resolution_clock::now();
    //pmf
    int count = 0;
    for(int i=0; i < pic.size();++i){
        histogram[pic[i]] += 1;
        count += 1;
    }
    std::cout << "Amount: " << count << std::endl;

    //cdf
    cdf[0] = histogram[0];
    for(int i=1; i < cdf.size();++i){
        cdf[i] = cdf[i-1] + histogram[i];
    }
    //got cdf -> search for minimum -> h(v)
    int V_MIN = 0;
    bool found = true;
    for(int i=0; found && i!=cdf.size();++i){
        if(histogram[i]!=0){
            V_MIN = cdf[i];
            found = false;
        }
    }
    //h_b here
    auto h_v_maker = [&](std::vector<int> cdf0, int index){
        int nominator = cdf0[index] - V_MIN;
        int denominator = (size1 * size2 - V_MIN);
        int val = (int)round(255*nominator/denominator);
        //std::cout << cdf0[index] << " | " << val <<  std::endl; 
        return val;
    };
    for(int i=0; i!=cdf.size(); ++i){
        h_v[i] = h_v_maker(cdf,i);
    }
    //print as how it will look!
    //for(int i=0; i < cdf.size();++i){
    //    std::cout << "# " << i << " : " << cdf[i] << " : " << histogram[i] << " H(v):" << h_v[i] << std::endl;
    //}
    std::cout << "H(v) created" << std::endl; 
    for(int i=0; i!=pic.size();++i){
        eq_pic[i] = h_v[pic[i]];
    }
    auto time1 = std::chrono::high_resolution_clock::now();
    std::cout << "Histogram Equalized under " << std::chrono::duration_cast<std::chrono::nanoseconds>(time1-time0).count()/pow(10,6) << " msec!"  << std::endl;
    save_pic(eq_pic);


    return 0;
}
