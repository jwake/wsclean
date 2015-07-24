#include "fftwmultithreadenabler.h"

#include <iostream>

#include <fftw3.h>
#include <unistd.h>

FFTWMultiThreadEnabler::FFTWMultiThreadEnabler(bool reportNrThreads)
{
	int threadCount = sysconf(_SC_NPROCESSORS_ONLN);
	if(reportNrThreads)
		std::cout << "Setting FFTW to use " << threadCount << " threads.\n";
	fftw_init_threads();
	fftw_plan_with_nthreads(threadCount);
}

FFTWMultiThreadEnabler::FFTWMultiThreadEnabler(size_t nThreads, bool reportNrThreads)
{
	if(reportNrThreads)
		std::cout << "Setting FFTW to use " << nThreads << " threads.\n";
	fftw_init_threads();
	fftw_plan_with_nthreads(nThreads);
}

FFTWMultiThreadEnabler::~FFTWMultiThreadEnabler()
{
	fftw_plan_with_nthreads(1);
	fftw_cleanup_threads();
}
