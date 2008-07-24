#include "datatypes.hh"
#include "io.hh"

#include <zlib.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

typedef std::map<term_t, freq_t> term_freq_map;
typedef std::vector< term_freq_map::const_iterator > term_list;
typedef std::vector< term_list > hashcode_term_map;


int main(int argc, char *argv[])
{
    /* Parse command line arguments */
    if(argc < 3)
    {
        std::cout << "usage: create <index file> <document identifier>" << std::endl;
        return 1;       
    }
    
    /* Read document identifier */
    doc_id_t di = atoll(argv[2]);
    if(di == 0)
    {
        std::cerr << "create: invalid document id \"" << argv[2] << "\"" << std::endl;
        return 1;       
    }
    
    /* Open output file */
    std::ofstream os(argv[1], std::ios::binary);
    if((!os) || (write_header(os), !os))
    {
        std::cerr << "create: invalid index file \"" << argv[1] << "\"" << std::endl;
        return 1;
    }


    term_t term;
    term_freq_map tfm;
    unsigned long long dl = 0;
    
    /* Read index terms; one on each line */    
    while(getline(std::cin, term))
    {
        /* FIXME: check for UTF-8 validness! */
        ++tfm[term];
        ++dl;
    }

    /* Create hash table for index terms */
    hashcode_term_map htm(1024);
    /* FIXME: pick a suitable hash table size based on number of terms */
    for(term_freq_map::const_iterator i = tfm.begin(); i != tfm.end(); ++i)
        htm[hash(i->first) % htm.size()].push_back(i);

    /* Write index */
    write_empty_hashtable(os, htm.size());
    hashcode_t code;
    for(code = 0; code < htm.size(); ++code)
    {
        write_hashtable_index(os, code);   
        for(term_list::const_iterator i = htm[code].begin(); i != htm[code].end(); ++i)
        {
            /* Compress document list */
            Bytef doclist_buf[16];
            uLongf doclist_length = sizeof(doclist_buf);
            compress(doclist_buf, &doclist_length, reinterpret_cast<Bytef*>(&di), sizeof(di));

            /* Compress term frequency list */
            Bytef tflist_buf[16];
            uLongf tflist_length = sizeof(tflist_buf);
            unsigned short tf = ( (*i)->second * 65535 ) / dl;
            tflist_length = sizeof(tflist_buf);
            compress(tflist_buf, &tflist_length, reinterpret_cast<Bytef*>(&tf), sizeof(tf));
            
            /* Write term entry */
            write_term_header(os, (*i)->first, doclist_length, tflist_length, 1);
            write_term(os, (*i)->first);
            os.write(reinterpret_cast<char*>(doclist_buf), doclist_length);
            pad(os);
            os.write(reinterpret_cast<char*>(tflist_buf), tflist_length);
            pad(os);
        }
    }
    write_hashtable_index(os, code);
}
