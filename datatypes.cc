#include "datatypes.hh"

hashcode_t hash(const term_t &input)  /* standard 32-bit FNV-1a hash */
{   
    hashcode_t code = 0x811C9DC5;
    for(term_t::const_iterator i = input.begin(); i != input.end(); ++i)
    {
        code ^= (unsigned char)*i;
        code *= 0x01000193;
    }
    return code;
}
