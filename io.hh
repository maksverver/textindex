#ifndef IO_HH
#define IO_HH

#include "datatypes.hh"
#include <fstream>
#include <vector>

typedef std::vector<std::ifstream::pos_type> hashtable_t;

/* Writing unsigned integers in network order */
bool write_uint8 (std::ofstream &os, unsigned char v);
bool write_uint16(std::ofstream &os, unsigned short v);
bool write_uint32(std::ofstream &os, unsigned long v);
bool write_uint64(std::ofstream &os, unsigned long long v);

/* Writing file format */
bool pad(std::ofstream &os);
bool write_header(std::ofstream &os);
bool write_empty_hashtable(std::ofstream &os, hashcode_t terms);
bool write_hashtable_index(std::ofstream &os, hashcode_t index);
bool write_term_header(std::ofstream &os, const term_t &term,
    unsigned long long dll, unsigned long long tfll, freq_t df);
bool write_term(std::ofstream &os, const term_t &term);


/* Data structures for read-only access */

class Index
{
public:
    class TermIterator
    {
        friend class Index;

    public:
        TermIterator();
        
        bool valid();
        void next();

        term_t term();
        freq_t doc_freq();
                
        inline operator bool() { return valid(); }
        inline TermIterator& operator++() { next(); return *this; }
    
    protected:
        TermIterator(int fd, off_t offset);
        void seek(off_t offset);
        
        int fd;
        off_t begin,
              ts_begin, ts_end,
              dl_begin, dl_end,
              tfl_begin, tfl_end,
              end;
        
        freq_t df;
    };

    Index(const char *filepath);
    ~Index();

    TermIterator iterator();
    TermIterator iterator(const term_t &term);
    
    inline operator bool() { return fd > 0; }
    inline TermIterator operator[] (const term_t &term) { return iterator(term); }

private:
    int fd;
    
    unsigned index_size;
    unsigned long long *index_entries;
};

#endif //ndef IO.HH
