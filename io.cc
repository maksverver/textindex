#include "io.hh"


extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
}

static const char magic[8] = { 'I', 'n', 'd', 'e', 'x', '\r', '\n', 26 };

bool write_uint8(std::ofstream &os, unsigned char v)
{
    os.put(v);
    return os;
}

bool write_uint16(std::ofstream &os, unsigned short v)
{
    return write_uint8(os, v >> 8) &&
           write_uint8(os, v & 0x00FF);
}

bool write_uint32(std::ofstream &os, unsigned long v)
{
    return write_uint16(os, v >> 16) &&
           write_uint16(os, v & 0x0000FFFF);
}

bool write_uint64(std::ofstream &os, unsigned long long v)
{
    return write_uint32(os, v >> 32) &&
           write_uint32(os, v & 0x00000000FFFFFFFF);
}

bool pad(std::ofstream &os) /* to 8-byte boundary */
{
    static const char zeroes[8] = { };
    
    std::ofstream::pos_type pos = os.tellp();
    if(pos%8 != 0)
        os.write(zeroes, 8 - pos%8);

    return os;
}

bool write_header(std::ofstream &os)
{
    return os.write(magic, 8) &&
           write_uint32(os,     16) &&     /* header length: 16 bytes */
           write_uint16(os, 0x0100) &&  /* header version: 1.0 */
           write_uint16(os,      0);    /* flags: 0 */
}

bool write_empty_hashtable(std::ofstream &os, hashcode_t terms)
{
    write_uint16(os, 0);
    write_uint16(os, 0);
    write_uint16(os, 0);
    write_uint16(os, terms);
    for(hashcode_t term = 0; term <= terms; ++term)
        write_uint64(os, 0);
    pad(os);
    
    return os;
}

bool write_hashtable_index(std::ofstream &os, hashcode_t index)
{
    std::ofstream::pos_type old_pos = os.tellp();
    
    return os.seekp(24 + 8*index) &&
           write_uint64(os, old_pos) &&
           os.seekp(old_pos);
}

bool write_term_header(std::ofstream &os, const term_t &term,
    unsigned long long dll, unsigned long long tfll, freq_t df)
{
    return write_uint32(os, 0) &&
           write_uint32(os, term.size()) &&
           write_uint64(os, dll) &&
           write_uint64(os, tfll) &&
           write_uint64(os, df);
}

bool write_term(std::ofstream &os, const term_t &term)
{
    return os.write(term.c_str(), term.length() + 1) &&
           pad(os);
}


bool align(std::ifstream &is) /* to 8-byte boundary */
{
    std::ofstream::pos_type pos = is.tellg();
    
    if(pos%8 != 0)
    {
        pos += 8 - pos%8;
        is.seekg(pos);
    }
        
    return is;    
}


inline unsigned short interpret_uint16(unsigned char buf[2])
{
    typedef unsigned short t;
    return ((t)buf[0] << 8) | ((t)buf[1] << 0);
}

inline unsigned long interpret_uint32(unsigned char buf[4])
{
    typedef unsigned long t;
    return ((t)buf[0] << 24) | ((t)buf[1] << 16) | ((t)buf[2] << 8) | ((t)buf[3] << 0);
}

inline unsigned long long interpret_uint64(unsigned char buf[8])
{
    typedef unsigned long long t;
    return ((t)buf[0] << 56) | ((t)buf[1] << 48) | ((t)buf[2] << 40) | ((t)buf[3] << 32) |
           ((t)buf[4] << 24) | ((t)buf[5] << 16) | ((t)buf[6] <<  8) | ((t)buf[7] <<  0);
}

Index::Index(const char *filepath)
{
    index_entries = NULL;
    
    fd = open(filepath, O_RDONLY);
    if(fd < 0)
        return;
    
    unsigned char buffer[24];
    static const unsigned char header[16] = {
        'I', 'n', 'd', 'e', 'x', '\r', '\n', 26, 0, 0, 0, 16, 1, 0, 0, 0 };
    
    if( read(fd, buffer, sizeof(buffer)) != sizeof(buffer) ||
        memcmp(buffer, header, sizeof(header)) != 0 )
    {
        close(fd);
        fd = -1;
        return;
    }
    
    index_size = interpret_uint16(buffer + 22);
    
    const ssize_t bytes = sizeof(unsigned long long)*(index_size+1);
    index_entries = (unsigned long long*)malloc(bytes);
    if(read(fd, index_entries, bytes) != bytes)
    {
        close(fd);
        fd = -1;
        free(index_entries);
        index_entries = NULL;
        return;
    }
}

Index::~Index()
{
    if(fd >= 0)
    {
        close(fd);
        free(index_entries);
    }
}

Index::TermIterator Index::iterator()
{
    return TermIterator(fd, 32 + 8*index_size);
}

Index::TermIterator Index::iterator(const term_t &term)
{
    /* TODO */
    return Index::TermIterator();
}


Index::TermIterator::TermIterator()
    : fd(-1)
{
}

inline off_t align(off_t offset)   /* to 8 byte boundary */
{
    while(offset%8 != 0)
        ++offset;
    return offset;
}

Index::TermIterator::TermIterator(int fd, off_t offset)
    : fd(fd)
{
    seek(offset);
}
        
bool Index::TermIterator::valid()
{
    return fd >= 0;
}

void Index::TermIterator::next()
{
    seek(end);
}

void Index::TermIterator::seek(off_t offset)
{
    unsigned char header[32];
    lseek(fd, offset, SEEK_SET);
    if(read(fd, header, sizeof(header)) != sizeof(header))
    {
        fd = -1;
        return;
    }
    
    begin     = offset;
    ts_begin  = offset + 32;
    ts_end    = ts_begin + interpret_uint32(header + 4);
    dl_begin  = align(ts_end);
    dl_end    = dl_begin + interpret_uint64(header + 8);
    tfl_begin = align(dl_end);
    tfl_end   = tfl_begin + interpret_uint64(header + 16);
    end       = align(tfl_end);
    
    df        = interpret_uint64(header + 24);
}

term_t Index::TermIterator::term()
{
    term_t result;
    result.resize(ts_end - ts_begin);
    if( lseek(fd, ts_begin, SEEK_SET) == (off_t)-1)
        perror("");
        
    if( lseek(fd, ts_begin, SEEK_SET) == ts_begin &&
        read(fd, &result[0], ts_end - ts_begin) == ts_end - ts_begin)
        return result;
    else
        return term_t();
}

freq_t Index::TermIterator::doc_freq()
{
    return df;
}
