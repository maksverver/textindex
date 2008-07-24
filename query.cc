#include "datatypes.hh"
#include "io.hh"

#include <iostream>

int main(int argc, char *argv[])
{
    /* Parse command line arguments */
    if(argc != 2 && argc != 3)
    {
        std::cout << "usage: query <index file> [query]" << std::endl;
        return 1;       
    }
    
    /* Open input file */
    Index index(argv[1]);
    if(!index)
    {
        std::cerr << "query: invalid index file \"" << argv[1] << "\"" << std::endl;
        return 1;       
    }
    
    /* Parse query */
    if(argc > 2)
    {
        std::string query(argv[2]);
        /* TODO */
    }
    else
    {
        Index::TermIterator ti = index.iterator();
        while(ti)
        {
            std::cout << ti.term() << " (" << ti.doc_freq() << " documents)" << std::endl;
            ti.next();
        }        
    }
}
