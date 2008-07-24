#ifndef TERMS_HH
#define TERMS_HH

#include <string>

typedef unsigned hashcode_t;
typedef std::string term_t;
typedef unsigned long long freq_t;
typedef unsigned long long doc_id_t;

hashcode_t hash(const term_t &input);

#endif // ndef TERMS_HH
