#include <cassert>
#include <iostream>
#include <thread>
#include <random>
#include <limits>
#include <chrono>

/* Taken as a reference from the book for this exercise */
class vector 
{
  public:
    vector(int size) : my_size(size), data(new double[size]) {}

    vector() : my_size(0), data(0) {}

    ~vector() { delete[] data; }

    vector(const vector& that) 
      : my_size(that.my_size), data(new double[my_size])
    {
	for (int i=0; i<my_size; ++i) { data[i] = that.data[i]; }
    }

    void operator=(const vector& that) 
    {
	assert(that.my_size == my_size);
	for (int i=0; i<my_size; ++i) 
	    data[i] = that.data[i];
    }

    int size() const { return my_size; }
    int size() { return my_size; }

    double& operator[](int i) const {
	assert(i>=0 && i<my_size);
	return data[i];
    }

    double& operator[](int i) {
	assert(i>=0 && i<my_size);
	return data[i];

    }

    vector operator+(const vector& that) const {
	assert(that.my_size == my_size);
	vector sum(my_size);
	for (int i= 0; i < my_size; ++i)
	    sum[i] = (*this)[i] + that[i]; 
	return sum;
    }

  private:
    int     my_size;
    double* data;
};

std::ostream& operator<<(std::ostream& os, const vector& v)
{
  os << '[';
  for (int i= 0; i < v.size(); ++i) os << v[i] << ',';
  os << ']';
  return os;
}

double dot(const vector& v, const vector& w) 
{
    double s;
    for (int i= 0; i < v.size(); i++)
	s+= v[i] * w[i];
    return s;
}

/* simple randomization */
std::default_random_engine& global_urng() {
    static std::default_random_engine u{};
    return u;
}

void randomize() {
    static std::random_device rd{};
    global_urng().seed(rd());
}

int pick(int from, int thru) {
    static std::uniform_int_distribution<> d{};
    using parm_t = decltype(d)::param_type;
    return d(global_urng(), parm_t{from, thru});
}

int main() {
    vector sizes(4);
    sizes[0] = 100; sizes[1] = 1000; sizes[2] = 10000; sizes[3] = 100000;
    
    for (int i = 0; i < sizes.size() - 1; i++) {
        vector v00(sizes[i]);
        vector v01(sizes[i]);
        vector res0(sizes[i]);
        vector v10(sizes[i+1]);
        vector v11(sizes[i+1]);
        vector res1(sizes[i+1]);

        for (int j = 0; j < sizes[i]; j++) {
            v00[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            v01[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            // v00[j] = pick(0, 10);
            // v01[j] = pick(0, 10);
        }
        for (int j = 0; j < sizes[i+1]; j++) {
            v10[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            v11[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            // v10[j] = pick(0, 10);
            // v11[j] = pick(0, 10);
        }

        std::chrono::time_point<std::chrono::high_resolution_clock> start0 = std::chrono::steady_clock::now();

        std::thread thread0 (
            [&res0, &v00, &v01] () {
                res0 = v00 + v01;
            });
        thread0.join();

        auto end0 = std::chrono::steady_clock::now();

        std::thread thread1 (
            [&res1, &v10, &v11] () {
                res1 = v10 + v11;
            });
        thread1.join();

        auto end1 = std::chrono::steady_clock::now();
        
        std::cout << "[" << res0[0] << ", " << res0[1] << ", ... , " << res0[res0.size()-2] << ", " << res0[res0.size()-1] << "]" << std::endl;
        std::cout << "[" << res1[0] << ", " << res1[1] << ", ... , " << res1[res1.size()-2] << ", " << res1[res1.size()-1] << "]" << std::endl;
        const auto thread0time = std::chrono::duration_cast<std::chrono::milliseconds>(end0 - start0).count();
        std::cout << "Calculation for " << sizes[i] << " vector took " << thread0time << std::endl;
        std::cout << "Calculation for " << sizes[i+1] << " vector took ";// << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - end0) << std::endl;
    }

  return 0;
}