#include <iostream>
#include <cmath>
#include <fstream>

//for parallellization
#include <vector>
#include <random>
#include <numeric>
#include <future>

int size1 = 0;
int size2 = 0;
int count = 0;
int V_MIN = 0;

int low = 0;
int high = 255;

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

//the main thing herec compared to basic is that i hae multiple threads
//this causes a bit of an issue as the first hist has to managed
//to not be accessed to write in it by multiple threads
// -> make a vector with atomic.
//no other element really requires as reading can be done by multiple threads
//no other element uses a higher element count object as its source of creation

// ------------------------------------------------------------------------
//i have to see how many threads i have -> it0 and it1 to these "bins"
//each thread always counts in this bin, never in other.

int main(int, char**) {
    //print program title
    std::cout << "Basic Histogramm Equalizer Started.\n";
    //init
    std::vector<int> pic;               //input pic
    std::vector<int> eq_pic;            //output pic

    std::vector<atomic_wrapper<int>> atomic_histogram(256); //his of pic
    std::vector<int> cdf(256,0);        //point mass function of pic
    std::vector<int> h_v(256,0);        //new values for eq_pic created from eq

    //i need dem threads!
    int max_num_of_threads = (int)std::thread::hardware_concurrency();
    std::vector<std::future<void>> atomic_futures(max_num_of_threads);
	std::cout << "Using " << max_num_of_threads << " threads.\n";


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

    //"lambdas"
    //histogramm from pic
    auto atomic_hist = [&](auto it0, auto it1){
        for(auto it=it0; it!=it1;++it){
            //as i take it from the pic, one cannot get values outside of [0,255]
            // no if...
            atomic_histogram[size_t(*it)].data += 1;
        }
    };
    //h_b here
    auto h_v_maker = [&](std::vector<int> cdf0, int index){
        int nominator = cdf0[index] - V_MIN;
        int denominator = (size1 * size2 - V_MIN);
        int val = (int)round(255*nominator/denominator);
        //std::cout << cdf0[index] << " | " << val <<  std::endl; 
        return val;
    };
    
    auto time0 = std::chrono::high_resolution_clock::now();
    //multithreaded
    //multithread: it from [it0,it1)
    //pmf
    for(int n=0; n<max_num_of_threads; ++n )
	{
		auto it0 = pic.begin() + n * pic.size() / max_num_of_threads;
		auto it1 = pic.begin() + (n + 1) * pic.size() / max_num_of_threads;
		atomic_futures[n] = std::async( std::launch::async, atomic_hist, it0, it1 );
	}
    //wait on futures and add results when it is possible
    std::for_each(atomic_futures.begin(), atomic_futures.end(), [](std::future<void>& f){ f.get(); } );
    int i = 0;
	for(auto& h : atomic_histogram)
	{   
		auto q = h.data.load();
		std::cout << i << " : " << q << std::endl;
		count += q;
        i++;
	}
    std::cout << "Count" << count << std::endl;


    /*
    std::cout << "Amount: " << count << std::endl;

    //this cannot be multithreaded easily
    //cdf
    cdf[0] = atomic_histogram[0].data;
    for(int i=1; i < cdf.size();++i){
        cdf[i] = cdf[i-1] + atomic_histogram[i].data;
    }

    //this cannot be multithreaded easily
    //got cdf -> search for minimum -> h(v)
    bool found = true;
    for(int i=0; found && i!=cdf.size();++i){
        if(atomic_histogram[i].data!=0){
            V_MIN = cdf[i];
            found = false;
        }
    }

    for(int i=0; i!=cdf.size(); ++i){
        h_v[i] = h_v_maker(cdf,i);
    }

    //multithread
    std::cout << "H(v) created" << std::endl; 
    for(int i=0; i!=pic.size();++i){
        eq_pic[i] = h_v[pic[i]];
    }
    auto time1 = std::chrono::high_resolution_clock::now();
    std::cout << "Histogram Equalized under " << std::chrono::duration_cast<std::chrono::nanoseconds>(time1-time0).count()/pow(10,6) << " msec!"  << std::endl;
    save_pic(eq_pic);
    */

    return 0;
}
