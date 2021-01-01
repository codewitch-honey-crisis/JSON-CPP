# JSON-CPP
JSON (C++) is a streaming JSON processor that introduces a navigation and extraction paradigm to efficiently search and pick data out of JSON documents of virtually any size, even on tiny machines with 8-bit processors and kilobytes of RAM.

The library provides something innovative - a streaming search and retrieve, called an "extraction". This allows you to front load several requests for related fields, array elements or values out of your document and queue them for retrieval in one highly efficient operation. It's not transactionally atomic because that's impossible for a pure streaming pull parser on a forward only stream, but it's as close as you're likely to get. When performing extractions, the reader automatically knows how to skip forward over data it doesn't need without loading it into RAM so using these extractions allows you to tightly control what you receive and what you don't without having to load field names or values into RAM just to compare or skip past them. You only need to reserve space enough to hold what you want you actually want to look at.

https://www.codeproject.com/Articles/5290815/JSON-by-Design-A-Walkthrough-of-a-High-Performance
