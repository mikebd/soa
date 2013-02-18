
#include "url.h"

#include "googleurl/src/gurl.h"
#include "googleurl/src/url_util.h"
#include "jml/arch/exception.h"
#include "jml/db/persistent.h"

#include <iostream>

using namespace std;

namespace Datacratic {


namespace {

struct Init {
    Init()
    {
        url_util::Initialize();
    }
} init;

}

Url::
Url()
    : url(new GURL())
{
}

Url::
Url(const std::string & s_)
    : original(s_)
{
    std::string s = s_;
    if (s == "") {
        url.reset(new GURL(s));
        return;
    }

    if (s.find("://") == string::npos) {
        s = "http://" + s;
    }
    url.reset(new GURL(s));

    if (url->possibly_invalid_spec().empty()) {
        //cerr << "bad parse 1" << endl;
        url.reset(new GURL("http://" + s));
        if (url->possibly_invalid_spec().empty()) {
            //cerr << "bad parse 2" << endl;
            url.reset(new GURL("http://" + s + "/"));
        }
    }
    
    //cerr << "parsing " << s << " returned " << toString()
    //     << " host " << host()
    //     << " valid " << valid()
    //     << endl;
    //cerr << "url has scheme " + gurl.scheme() << endl;
    //cerr << "url has spec " + gurl.spec() << endl;
}

Url::
~Url()
{
}

std::string
Url::
toString() const
{
    if (valid())
        return canonical();
    return original;
}

const char *
Url::
c_str() const
{
    if (valid())
        return url->spec().c_str();
    return original.c_str();
}

bool
Url::
valid() const
{
    return url->is_valid();
}

bool
Url::
empty() const
{
    return url->is_empty();
}

std::string
Url::
canonical() const
{
    if (!valid()) return "";
    return url->spec();
}

std::string
Url::
scheme() const
{
    return url->scheme();
}

std::string
Url::
host() const
{
    return url->host();
}

bool
Url::
hostIsIpAddress() const
{
    return url->HostIsIPAddress();
}

bool
Url::
domainMatches(const std::string & str) const
{
    return url->DomainIs(str.c_str(), str.length());
}

int
Url::
port() const
{
    return url->IntPort();
}

std::string
Url::
path() const
{
    return url->path();
}

std::string
Url::
query() const
{
    return url->query();
}

uint64_t
Url::
urlHash()
{
    throw ML::Exception("urlHash");
}

uint64_t
Url::
hostHash()
{
    throw ML::Exception("hostHash");
}

void
Url::
serialize(ML::DB::Store_Writer & store) const
{
    unsigned char version = 0;
    store << version << original;
}

void
Url::
reconstitute(ML::DB::Store_Reader & store)
{
    unsigned char version;
    store >> version;
    if (version != 0)
        store >> original;
    *this = Url(original);
}

} // namespace Datacratic
