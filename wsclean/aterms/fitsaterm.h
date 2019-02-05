#ifndef FITS_ATERM_H
#define FITS_ATERM_H

#include "atermbase.h"

#include "../fitsreader.h"

#include "../uvector.h"

#include <complex>
#include <map>
#include <memory>
#include <vector>

/**
 * Class that reads in FITS images and resamples them onto aterm grids.
 * The fits file is supposed to have a TIME and FREQ axis. 
 */
class FitsATerm : public ATermBase
{
public:
	FitsATerm(size_t nAntenna, size_t width, size_t height, double ra, double dec, double dl, double dm, double phaseCentreDL, double phaseCentreDM);
	
	void OpenTECFile(const std::string& filename);
	void OpenDiagGainFile(const std::string& filename);
	
	virtual bool Calculate(std::complex<float>* buffer, double time, double frequency);
	
private:
	enum Mode { TECMode, DiagonalMode } _mode;
	
	void readImages(std::complex<float>* buffer, size_t timeIndex, double frequency);
	
	void resample(double* dest, const double* source);
	
	void evaluateTEC(std::complex<float>* dest, const double* source, double frequency);
	
	void copyToRealPolarization(std::complex<float>* dest, const double* source, size_t polIndex);
	void copyToImaginaryPolarization(std::complex<float>* dest, const double* source, size_t polIndex);
	void setPolarization(std::complex<float>* dest, size_t polIndex, std::complex<float> value);
	
	size_t _nAntenna, _nFrequencies, _width, _height;
	double _ra, _dec, _dl, _dm, _phaseCentreDL, _phaseCentreDM;
	std::vector<double> _timesteps;
	std::map<double, std::vector<std::complex<float>>> _bufferCache;
	ao::uvector<double> _scratch;
	size_t _curTimeindex;
	std::unique_ptr<FitsReader> _reader;
};

#endif
