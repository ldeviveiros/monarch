#ifndef __MONARCH_HPP
#define __MONARCH_HPP

#include "MonarchIO.hpp"
#include "MonarchTypes.hpp"
#include "MonarchExceptions.hpp"
#include "MonarchHeader.hpp"

class Monarch {
private:
  // Header for file
  MonarchHeader* hdr;

  // Default constructor is private, naturally
  Monarch();

  // Filename we are currently working with
  std::string filename;
  std::size_t recsize;

  // The MonarchIO class is the file pointer abstraction that we use
  // to read and write data.
  MonarchIO* io;

  // A const pointer to an internal MonarchSerializer type templated to
  // the MonarchRecord type.  
  char* const mem;
  MonarchRecord* const rec;

  // The simplest constructor takes a string filename, an access mode, and
  // an acquisition mode which determines the number of channels in the data
  // stream.
  Monarch(std::string filename, 
	  AccessMode iomode, 
	  AcquisitionMode datamode,
	  std::size_t DataWidth);

  // Specialized open methods
  static Monarch* OpenForReading(std::string filename);
  static Monarch* OpenForWriting(std::string filename);

public:
  ~Monarch();

  // Some bits for configuration flags.
  static const AcquisitionMode OneChannel;
  static const AcquisitionMode TwoChannel = 0x04;

  // Static factory method for constructing Monarch objects from a file.
  static Monarch* Open(std::string filename, 
		       AccessMode iomode, 
		       AcquisitionMode datamode);

  // The most basic constructor only needs a filename and an access mode.
  // In read mode this assumes one channel!
  static Monarch* Open(std::string filename, AccessMode iomode);

  // Here's a really sweet one - open using only a header.  This is of course
  // only for writing, 
  static Monarch* Open(MonarchHeader& hdr);

  // Close an open monarch file.
  bool Close();

  // Get the header
  MonarchHeader* GetHeader();

  // A RecordFactory - a factory method which builds properly initialized 
  // MonarchRecords.
  static MonarchRecord* NewRecord(std::size_t rsize);

  // Write a MonarchRecord to disk.  Assumes that the record is the right size!
  // To avoid disappointment, generate your records with Monarch::NewRecord.
  bool WriteRecord(MonarchRecord* rec);

  // Read the next event on disk into the MonarchSerializer buffer and return
  // a pointer to the value member of the union.  This exposes a MonarchRecord*
  // to the client.
  MonarchRecord* GetNextEvent();

};

#endif
