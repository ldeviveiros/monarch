#include "mantis_egg.hpp"

mantis_egg::mantis_egg() 
  : file_ptr(NULL),
    header_finished(false),
    file_name("default_egg.egg"),
    data_size(sizeof(unsigned char)),
    data_width(10)
{ /* no-op */ }

mantis_egg::~mantis_egg()
{
  if(file_ptr) {
    fclose(file_ptr);
  }
}

mantis_egg* mantis_egg::egg_from_env()
{
  mantis_egg* egg_ptr = new mantis_egg();
  egg_ptr->file_ptr = fopen(egg_ptr->file_name.c_str(),"wb");

  egg_ptr->add_header_attr("clock_ticks_per_second",
			   CLOCKS_PER_SEC);

  if(egg_ptr->file_ptr == NULL) {
    delete egg_ptr;
    egg_ptr = NULL;
  }
  return egg_ptr;
}

std::string mantis_egg::attr_to_xml(egg_hdr_k_type key,
				    egg_hdr_v_type val)
{
  std::stringstream fuse;
  fuse << "<key>" 
       << "name=\"" 
       << key 
       << "\" " 
       << "value=\"" 
       << val 
       << "\"</key>";
  return fuse.str();
}

std::string mantis_egg::xml_vers_header()
{
  return "<?xml version=\"1.0\"?>";
}

std::string mantis_egg::xml_hdr_open()
{
  return "<header>";
}

std::string mantis_egg::xml_hdr_close()
{
  return "</header>";
}

bool mantis_egg::write_raw_bytes(const void* tgt, 
				 std::size_t tgt_size, 
				 std::size_t tgt_width)
{
  bool res = true;

  std::size_t written = fwrite(tgt,
			       tgt_size,
			       tgt_width,
			       this->file_ptr);
  if ( written != tgt_size * tgt_width ) {
    res = false;
  }

  return res;
}

bool mantis_egg::write_header()
{
  bool ret_val = true;
  std::string hdr_string;
  std::stringstream fuse;
  egg_hdr_container::iterator header_it = this->header_attrs.begin();

  if( fseek(this->file_ptr,0,SEEK_SET) ) {
    ret_val = false;
  }
  
  if( ret_val ) {
    fuse << mantis_egg::xml_vers_header();
    fuse << mantis_egg::xml_hdr_open();

    // Write header attributes
    while( header_it != header_attrs.end() ) {
      std::string xml = mantis_egg::attr_to_xml((*header_it).first,
						(*header_it).second);
      fuse << xml;
      header_it++;
    }

    // Now encode the data size and features, and write it as a string.
    std::stringstream dataDesc;
    dataDesc << "id:"
	     << sizeof(MantisData::IdType) << "|"
	     << "ts:"
	     << sizeof(MantisData::ClockType) << "|"
	     << "data:"
	     << this->data_width;
    fuse << mantis_egg::attr_to_xml("data_format",dataDesc.str());

    // Close the header, we're done.
    fuse << mantis_egg::xml_hdr_close();
  }

  char buffer[2*sizeof(std::size_t)];
  sprintf(buffer,
	  "%x",
	  (unsigned)fuse.str().length());

  this->write_raw_bytes(buffer,
			sizeof(unsigned char),
			strlen(buffer));

  this->write_raw_bytes(fuse.str().c_str(),
			sizeof(unsigned char),
			fuse.str().length());

  if( ret_val ) {
    this->header_finished = true;
  }

  return ret_val;
}

bool mantis_egg::write_data(MantisData* block)
{
  /* fmt: |fID|fTick|fData| */
  u_long_byter.value = block->fId;
  clock_t_byter.value = block->fTick;
  this->write_raw_bytes(u_long_byter.value_bytes,
			u_long_byter.value_bytes[0],
			sizeof(u_long_byter));
  this->write_raw_bytes(clock_t_byter.value_bytes,
			clock_t_byter.value_bytes[0],
			sizeof(clock_t_byter));
  this->write_raw_bytes(block->fDataPtr,
			this->data_size,
			this->data_width);
  return true;
}
