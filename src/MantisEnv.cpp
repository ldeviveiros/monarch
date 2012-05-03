#include "MantisEnv.hpp"

MantisEnv::MantisEnv()
  : outName("mantis_out.egg"), // default output name
    clockRate(500.0),          // default ADC clock rate (MHz)
    runLength(600000),            // default run length in milliseconds
    dataWidth(4194304),        // number of bytes in a single record
    bufferCount(630)           // number of circular buffer nodes
{ /* no-op */ }

void MantisEnv::setOutName(std::string newOutName)
{
  (*this).outName = newOutName;
  return;
}

std::string MantisEnv::getOutName()
{
  return (*this).outName;
}

void MantisEnv::setClockRate(std::string clockRateStr)
{
  try {
    std::istringstream(clockRateStr) >> (*this).clockRate;
  }
  catch (std::exception& e) {
    throw new clock_rate_exception(clockRateStr);
  }
  return;
}

double MantisEnv::getClockRate()
{
  return (*this).clockRate;
}

void MantisEnv::setRunLength(std::string runLengthStr)
{
  try {
    std::istringstream(runLengthStr) >> (*this).runLength;
  }
  catch (std::exception& e) {
    throw new run_length_exception(runLengthStr);
  }

  return;
}

std::size_t MantisEnv::getRunLength()
{
  return (*this).runLength;
}

void MantisEnv::setDataWidth(std::string dWidthStr)
{
  std::istringstream(dWidthStr) >> (*this).dataWidth;
  return;
}

std::size_t MantisEnv::getDataWidth()
{
  return (*this).dataWidth;
}

void MantisEnv::setBufferCount(std::string bufCountStr)
{
  std::istringstream(bufCountStr) >> (*this).bufferCount;
  return;
}

std::size_t MantisEnv::getBufferCount()
{
  return (*this).bufferCount;
}

safeEnvPtr MantisEnv::parseArgs(int argc, char** argv)
{
  safeEnvPtr result(new MantisEnv());
 
  int c;
  try {
    while((c = getopt(argc,argv,"o:r:d:w:c:")) != -1) {    
      switch(c) {
      case 'o':
	result->setOutName(optarg);
	break;
      case 'r':
	result->setClockRate(optarg);
	break;
      case 'd':
	result->setRunLength(optarg);
	break;
	  case 'w':
	result->setDataWidth(optarg);
	break;
	  case 'c':
	result->setBufferCount(optarg);
	break;
      default:
	throw new argument_exception();
      }
    }
  }
  catch(argument_exception* e) {
    throw e;
  }
  return result;
}

void MantisEnv::verifyEnvironment(safeEnvPtr someEnvironment)
{
  return;
}

std::ostream& operator << (std::ostream& outstream, safeEnvPtr& obj)
{
  outstream << "output file name: " << (obj.get())->getOutName() << "\n"
	    << "digitizer rate: " << (obj.get())->getClockRate() << "(MHz)\n"
	    << "run length: " << (obj.get())->getRunLength() << "(s)\n"
	    << "data width: " << (obj.get())->getDataWidth() << "(bytes)\n"
	    << "buffer count: " << (obj.get())->getBufferCount() << "(entries)\n";
    
  return outstream;
}
