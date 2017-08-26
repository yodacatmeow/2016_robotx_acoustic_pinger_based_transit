// Headers; Framework (Aquila)
#include "aquila/global.h"
#include "aquila/transform/FftFactory.h"
#include <algorithm>
#include <functional>
#include <memory>
// Headers; DSP; read & write data
#include <cstdlib>	// atof ('stof' is not supported)
#include <iostream>
#include <fstream>	// ifstream
#include <string>
#include <cmath>	// abs, cosine
// Headers; TCP/IP socket Comm. (related codes are removed)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef unsigned char byte; // 8bits (= 1 byte) for TCP/IP socket.
using namespace std;

// Constants; parameters
const double PI = 3.141592653589793238460;	// Phi
const std::size_t SIZE = 524288;	        // Sample length (it should be power of 2); 2^19 = 524288
const double FREQ = 200000;                 // Sampling frequency [Hz]

// Sample arrays
double* ch0_raw = new double[SIZE];	// Receiver Ch_0
double* ch1_raw = new double[SIZE];	// Receiver Ch_1
double* ch0 = new double[SIZE];	    // Band-pass_filter(Ch0_raw)
double* ch1 = new double[SIZE];     // Band-pass_filter(Ch1_raw)
double TDoA0;					    // Time difference of arrival (TDoA) between the two channels

// Generate a Hanning window
// HanningWin(output_array, window length)
double* HanningWin(double* h, std::size_t win_size)
{
	for(std::size_t i = 0; i < win_size; i++)
	{
	    h[i] = 0.5 - 0.5 * cos( (2 * (double) i * PI) / ((double) win_size) );	// Hann function
	}
	return 0;
}

// 1d-convolution of an array with a window
// convolve_1d(input_array, len(input_array), output_array, window, len(window))
double* convolve_1d (double* samp, std::size_t samp_size, double* convolve, double* win, std::size_t win_size)
{
	for(std::size_t i = 0; i <= samp_size - win_size; i++)
	{
		convolve[i] = 0;
	    for (std::size_t k = 0; k < win_size; k++)
	    {
	    	convolve[i] += abs(samp[i + k]) * win[win_size - k - 1];	// <- absolute value added
	    }
	}
	return 0;
}

// Find the maximum value in an array
// findMax(array, len(array))
double findMax(double arr[], std::size_t arrSize)
{
	double tmp = 0;
	for(int i = 0; i < arrSize; i++)
	{
		if(arr[i] > tmp)
		{
			tmp = arr[i];
		}
	}
	return tmp;
}

// Find an onset location very roughly; it returns position of the first value > 'thresholdVal'
// it is used for setting start of region of interest (ROI)
// findOnset(array, len(array), threshold)
double findOnset(double arr[], std::size_t arrSize, double thresholdVal)
{
	std::size_t onset;
	for(std::size_t i = 0; i < arrSize; i++)
			{
				if (arr[i] > thresholdVal)
				{
					if( i < (std::size_t) round(0.05 / (1 / FREQ)) )		// if onset is located at t < 0.05s
					{
						i = i + (std::size_t) round( 0.1 / (1 / FREQ) );	// Move 'i' to 0.1s later; find the next onset
					}
					else
					{
						onset = i;
						i = arrSize;
					}

				}
			}
	return onset;
}

// This function is very similar with 'findOnset'; But it does not find the next onset when onset is located at t < 0.05s
double findOnset2(double arr[], std::size_t arrSize, double thresholdVal)
{
	std::size_t onset;
	for(std::size_t i = 0; i < arrSize; i++)
			{
				if (arr[i] > thresholdVal)
				{
					onset = i;
					i = arrSize;
				}
			}
	return onset;
}

int main()
{
    // This block is for simulation with "sample.dat" -----------------------------------------------------------------
    ifstream ifile;	// Input file stream
    string line; 	// Buffer for reading a line in .dat file
    string temp;	// Buffer for an extracted value in 'line'

    ifile.open("sample.dat");
    if (ifile.is_open())
    {
    	for(std::size_t i = 0; i < SIZE; i++)
    	{
    		getline(ifile, line, '\n');	// Read a line
    		size_t len_line = 0;	// Length of a line
    		size_t posTab = 0;		// Position of '\t'
    		len_line = line.length();
    		posTab = line.find('\t');
    		temp = line.substr(0, posTab);				// [ch0_value]
    		ch0_raw[i] = atof( temp.c_str() );
    		temp = line.substr(posTab + 1, len_line);	// [ch1_value]
    		ch1_raw[i] = atof( temp.c_str() );
    	}
    }
    ifile.close();
    // The end of the block---------------------------------------------------------------------------------------------
    
    // Copy sample arrays
    for(std::size_t i = 0; i < SIZE; i++)
	{
		ch0[i] = ch0_raw[i];
		ch1[i] = ch1_raw[i];
	}

    // Band-pass filtering with Aquila
	const Aquila::FrequencyType sampleFreq = 200000;	// sampling freq. [Hz]
	const Aquila::FrequencyType df = 2000;				// delta_f

	const Aquila::FrequencyType f1_n1 = 32000;		    // Transmitter's freq. [Hz]
	const Aquila::FrequencyType f1_n2 = 64000;		    // second harmonic of 'f1_n1'

	const Aquila::FrequencyType f1_n1_L = f1_n1 - df;	// Lower freq. of pass-band_1
	const Aquila::FrequencyType f1_n1_H = f1_n1 + df;	// Upper freq. of pass-band_1
	const Aquila::FrequencyType f1_n2_L = f1_n2 - df;	// Lower freq of pass-band_2
	const Aquila::FrequencyType f1_n2_H = f1_n2 + df; 	// Lower freq of pass-band_2

    // Generate pass-band spectrum
	Aquila::SpectrumType filterSpectrum(SIZE);
	for (std::size_t i = 0; i < SIZE; i++)
    	    {
    	        if (
    	        		(  (i > (SIZE * f1_n1_L / sampleFreq)) && (i < (SIZE * f1_n1_H / sampleFreq))  )
    	        	 || (  (i > (SIZE * f1_n2_L / sampleFreq)) && (i < (SIZE * f1_n2_H / sampleFreq))  )
					)
    	        {
    	        	filterSpectrum[i] = 1.0;	// pass-band
    	        }
    	        else
    	        {
    	        	filterSpectrum[i] = 0.0;	// stop-band
    	        }
    	    }

    // Ch0 filtering
	auto fft = Aquila::FftFactory::getFft(SIZE);    // FFT
	Aquila::SpectrumType spectrum = fft->fft(ch0);
	// the following call does the multiplication of two spectra (complementary to convolution in time domain)
	std::transform
	(
		std::begin(spectrum),
		std::end(spectrum),
		std::begin(filterSpectrum),
		std::begin(spectrum),
		[] (Aquila::ComplexType x, Aquila::ComplexType y) { return x * y; }
	);
	// Inverse FFT moves us back to time domain
	fft->ifft(spectrum, ch0);

    // Scale change to [0,1]
	double maxCh0 = findMax(ch0, SIZE);
	for(std::size_t i = 0; i < SIZE; i++)
	{
		ch0[i] = ch0[i] / maxCh0;
	}
		
    // Find a location of an onset very roughly
	std::size_t onSetU = findOnset(ch0, SIZE, 0.9);	// Onset position (ch0)
	std::size_t onSetUL = onSetU - 1000;			// '1000' seems suitable for fs = 200k
	std::size_t onSetUH = onSetU + 1000;
		
    // Extract ROI (Range of Interest) which reduces computational complexity
	double* ROIch0 = new double[SIZE];
	double* ROIch1 = new double[SIZE];
	for(std::size_t i = 0; i < onSetUH - onSetUL + 1; i++)
	{
		ROIch0[i] = ch0[onSetUL + i];
		ROIch1[i] = ch1[onSetUL + i];
	}
		
    // Ch1 filtering
	spectrum = fft->fft(ROIch1);    // FFT
	std::transform
	(
		std::begin(spectrum),
		std::end(spectrum),
		std::begin(filterSpectrum),
		std::begin(spectrum),
		[] (Aquila::ComplexType x, Aquila::ComplexType y) { return x * y; }
	);
	fft->ifft(spectrum, ROIch1);    //IFFT

    // Hanning window
	std::size_t winSize = 30;
	double* h =  new double[winSize];
	HanningWin(h, winSize);

    // 1d-convolution of ch* with Hanning window
	double* ROIch0Red = new double[onSetUH - onSetUL + 1];
	double* ROIch1Red = new double[onSetUH - onSetUL + 1];
	convolve_1d(ROIch0, onSetUH - onSetUL + 1, ROIch0Red, h, winSize);
	convolve_1d(ROIch1, onSetUH - onSetUL + 1, ROIch1Red, h, winSize);

    // Find the maxim value in array
	maxCh0 = findMax(ROIch0Red,onSetUH - onSetUL + 1 - winSize);
	maxCh1 = findMax(ROIch1Red,onSetUH - onSetUL + 1 - winSize);
		
    // Scale change to [0,1]
	for(std::size_t i = 0; i < onSetUH - onSetUL + 1 - winSize; i++)
	{
		ROIch0Red[i] = ROIch0Red[i] / maxCh0;
		ROIch1Red[i] = ROIch1Red[i] / maxCh1;
	}	

    // Thresholding, Onset position finding
	double thrs = 0.35;	// threshold value
	std::size_t onSet0 = findOnset2(ROIch0Red, onSetUH - onSetUL + 1 - winSize, thrs);	// Onset position (ch0)
	std::size_t onSet1 = findOnset2(ROIch1Red, onSetUH - onSetUL + 1 - winSize, thrs);	// Onset position (ch1)

    // Time difference of Arrival (TDoA)
	double TDoA  = (double) onSet1 - (double) onSet0;

    // Saturation
	if(TDoA < -206)	    //Lower saturation (@ 200 kHz, d = 1.55)
	{TDoA0 = -206;}
	else if(TDoA > 206)	//Upper saturation (@ 200 kHz, d = 1.55)
	{TDoA0 = 206;}
	else if(TDoA == 0)
	{TDoA0 = 1;}
	else
	{TDoA0 = TDoA;}

    // Print 'TDoA0'
	cout << TDoA0;

    return 0;  
}