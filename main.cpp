#include <stdio.h>
// for perf
#include <chrono>
using namespace std::chrono;

// include JsonTree.h before JsonReader.h if you're going to use both
#include "src/JsonTree.h"
#include "src/JsonReader.h"
#include "src/FileLexContext.h"

void dumpFilters(bool shouldFilter=true,bool useStringPooling=false);
void dumpState(JsonReader jsonReader);
void dumpFile();
void dumpAllIds();
void dumpProductionCompanies();
void dumpShowName();
void dumpEpisodes(bool useStringPooling=false);
StaticFileLexContext<2048> fileLC;
StaticMemoryPool<1024*1024> pool;
StaticMemoryPool<1024*1024> stringPool;
int main()
{
    
    //dumpFilters(false);
    dumpFilters(true);
    //dumpFile();
    //dumpProductionCompanies();
    //dumpShowName();
    //dumpEpisodes(false);
    //dumpAllIds();

}
void dumpFilters(bool shouldFilter,bool useStringPooling) {
    if (!fileLC.open("./data.json"))
    {
        printf("Json file not found\r\n");
        return;
    }
    pool.freeAll();
    stringPool.freeAll();
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    JsonReader jsonReader(fileLC);

    const char* showfields[] = {"id","name","created_by"};
    const char* creditfields[] = {"name","profile_path"};
    JsonParseFilter filter(showfields,3,JsonParseFilter::WhiteList);
    // we're creating a nested filter for created_by:
    JsonParseFilter createdByFilter(creditfields,2,JsonParseFilter::WhiteList);
    // must be the same count and order as the parents fields:
    JsonParseFilter* subfilters[] {nullptr,nullptr,&createdByFilter}; 
    filter.pfilters = subfilters;
    JsonElement *pe = jsonReader.parseSubtree(pool,(shouldFilter)?&filter:nullptr,useStringPooling?&stringPool:nullptr);
    if(JsonReader::Error==jsonReader.nodeType()) {
        printf("\r\nError (%d): %s\r\n\r\n",jsonReader.lastError(),jsonReader.value());
        
    }
    milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    printf("Parsed %llu characters in %d milliseconds\r\n",fileLC.position()+1,(int)(end.count()-start.count()));
    printf("Used pool total: %llu\r\n",(unsigned long long)pool.used());
    printf("Used string pool total: %llu\r\n",(unsigned long long)stringPool.used());
    printf("Used total: %llu\r\n\r\n",(unsigned long long)stringPool.used()+(unsigned long long)pool.used());
    
    if(shouldFilter)  {
        if(nullptr!=pe) {
            char* sz = pe->toString(pool);
            if(nullptr!=sz) {
                printf("%s\r\n",sz);
            }
        }
    } else {
        printf("approx 200kB of result data omitted.\r\n");
    }
    printf("\r\n\r\n");
    fileLC.close();
}
void dumpFile() {
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    JsonReader jsonReader(fileLC);
    long long int nodes = 0; // we don't count the initial node
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // pull parsers return portions of the parse which you retrieve
    // by calling their parse/read method in a loop.
    bool done = false;
    while (!done && jsonReader.read())
    {
        ++nodes;
        // what kind of JSON element are we on?
        switch (jsonReader.nodeType())
        {
        case JsonReader::Value: // we're on a scalar value
            printf("Value ");
            switch (jsonReader.valueType())
            {                        // what type of value?
            case JsonReader::String: // a string!
                printf("String: ");
                jsonReader.undecorate();              // remove all the nonsense
                printf("%s\r\n", jsonReader.value()); // print it
                break;
            case JsonReader::Number:                                 // a number!
                printf("Number: %f\r\n", jsonReader.numericValue()); // print it
                break;
            case JsonReader::Boolean: // a boolean!
                printf("Boolean: %s\r\n", jsonReader.booleanValue() ? "true" : "false");
                break;
            case JsonReader::Null: // a null!
                printf("Null: (null)\r\n");
                break;
            }
            break;
        case JsonReader::Field: // this is a field
            printf("Field %s\r\n", jsonReader.value());
            break;
        case JsonReader::Object: // an object start {
            printf("Object (Start)\r\n");
            break;
        case JsonReader::EndObject: // an object end }
            printf("Object (End)\r\n");
            break;
        case JsonReader::Array: // an array start [
            printf("Array (Start)\r\n");
            break;
        case JsonReader::EndArray: // an array end ]
            printf("Array (End)\r\n");
            break;
        case JsonReader::Error: // a bad thing
            // maybe we ran out of memory, or the document was poorly formed
            printf("Error: (%d) %s\r\n", jsonReader.lastError(), jsonReader.value());
            done=true;
            break;
        }
    }
    milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    printf("Scanned %lli nodes and %llu characters in %d milliseconds\r\n",nodes,fileLC.position()+1,(int)(end.count()-start.count()));
    fileLC.close();
}
void dumpEpisodes(bool useStringPooling) {
    // make sure we are using fresh pools
    pool.freeAll();
    stringPool.freeAll();
    
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    // create a JSON reader
    JsonReader jsonReader(fileLC);

    // prepare a simple filter
    const char* fields[] = {"season_number","episode_number","name"};
    JsonParseFilter filter(fields,3,JsonParseFilter::WhiteList);
    // create some room to hold our return values
    JsonElement* values[3];
    filter.pvalues=values;

    // for tracking 
    size_t max = 0;
    size_t episodes=0;
    
    // yeah, we're timing it for fun
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    
    // skip to each field in the document named "episodes" regardless of where. 
    // JsonReader:All cuts through the heirarchy so running it repeatedly starting 
    // from the root is like doing $..episodes in JSONPath:
    // (jsonReader.read() advances to the field's value, in this case an array)
    while (jsonReader.skipToField("episodes", JsonReader::All) && jsonReader.read() ) {
        // if it's an array let's move to the first element using read():
        if(JsonReader::Array==jsonReader.nodeType() && jsonReader.read()) {
            // while we still have more array elements...
            while(JsonReader::EndArray!=jsonReader.nodeType()) {     
                // now parse that array element (an episode)
                // parse with a filter in place. We only want season_number,episode_number and name:
                JsonElement *pr= jsonReader.parseSubtree(pool,&filter,useStringPooling?&stringPool:nullptr);
                if(nullptr==pr) {
                    printf("\r\nError (%d): %s\r\n\r\n",jsonReader.lastError(),jsonReader.value());
                    return;
                } else {
                    // since we've whitelisted, the API filled in the values for us. It's easy to retrieve them
                    // normally you'd want to do checking here top.
                    printf("S%02dE%02d %s\r\n",
                        nullptr!=filter.pvalues[0]?(int)filter.pvalues[0]->integer():0,
                        nullptr!=filter.pvalues[1]?(int)filter.pvalues[1]->integer():0,
                        nullptr!=filter.pvalues[2]?filter.pvalues[2]->string():"(not found)");
                    // increase the episode count
                    ++episodes;
                }
                // track the max amount of data we end up using at one time
                if(pool.used()>max)
                    max=pool.used();
                // recycle this for the next query
                pool.freeAll();
            }
        
        } else if(JsonReader::Error==jsonReader.nodeType()) {
            break;
        }
    }
    if(JsonReader::Error==jsonReader.nodeType()) {
        // report the error
        printf("\r\nError (%d): %s\r\n\r\n",jsonReader.lastError(),jsonReader.value());
        return;
    }
    // stop the "timer"
    milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // report the statistics
    printf("Max used bytes of pool: %d\r\nScanned %d episodes and %llu characters in %d milliseconds\r\n",(int)max,(int)episodes,fileLC.position()+1,(int)(end.count()-start.count()));
    if(useStringPooling) {
        printf("Used string pool total: %llu\r\n",(unsigned long long)stringPool.used());
        printf("Used total: %llu\r\n\r\n",(unsigned long long)stringPool.used()+(unsigned long long)max);
    }
    // always close the file
    fileLC.close();

}

void dumpProductionCompanies() {
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    // create the reader
    JsonReader jsonReader(fileLC);
    // keep track of the number we find
    size_t count = 0;
    // time it for fun
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // find the production_companies field on this level of the hierarchy, than read to its value
    if (jsonReader.skipToField("production_companies",JsonReader::Siblings) && jsonReader.read()) {
        // we want only the names tha are under the current object. we have to pass a depth tracker
        // to tell the reader where we are.
        unsigned long int depth=0;
        while (jsonReader.skipToField("name", JsonReader::Descendants,&depth) && jsonReader.read()) {
            ++count;
            printf("%s\r\n",jsonReader.value());
        }
    }
    // stop the "timer"
    milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // dump the results
    printf("Scanned %llu characters and found %d companies in %d milliseconds\r\n",fileLC.position()+1,(int)count, (int)(end.count()-start.count()));

    // always close the file
    fileLC.close();
}
void dumpShowName() {
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    // create the reader
    JsonReader jsonReader(fileLC);
    // time it, for fun.
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    
    // we start on nodeType() Initial, but that's sort of a pass through node. skipToField()
    // simply reads through when it finds it, so our first level is actually the root object
    // look for the field named "name" on this level of the heirarchy.
    // then read its value which comes right after it.
    if ( jsonReader.skipToField("name",JsonReader::Siblings) && jsonReader.read()) {
        // deescape the string
        jsonReader.undecorate(); 
        printf("%s\r\n",jsonReader.value());
    } else if(JsonReader::Error==jsonReader.nodeType()) {
        // if we error out, print it
        printf("\r\nError (%d): %s\r\n\r\n",jsonReader.lastError(),jsonReader.value());
        return;
    }
    // stop the "timer"
    milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // print the results
    printf("Scanned %llu characters in %d milliseconds\r\n",fileLC.position()+1,(int)(end.count()-start.count()));
    // close the file
    fileLC.close();
}
void dumpAllIds() {
    // open the file
    if (!fileLC.open("./data.json"))
    {
        printf("Json file not found\r\n");
        return;
    }
    JsonReader jsonReader(fileLC);

    size_t ids=0;
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    while (jsonReader.skipToField("id",JsonReader::All) && jsonReader.read()) {
        printf("%lli\r\n",jsonReader.integerValue());
        ++ids;
    } 
    milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    printf("Scanned %d ids and %llu characters in %d milliseconds\r\n",(int)ids,fileLC.position()+1,(int)(end.count()-start.count()));
    fileLC.close();
}
void dumpState(JsonReader jsonReader) {
    switch(jsonReader.nodeType()) {
        case JsonReader::Error:
            printf("Error: (%d) %s\r\n",jsonReader.lastError(),jsonReader.value());
            break;
        case JsonReader::EndDocument:
            printf("End Document\r\n");
            break;
        case JsonReader::Initial:
            printf("Initial\r\n");
            break;
        case JsonReader::Value:
            switch(jsonReader.valueType()) {
                case JsonReader::String:
                    printf("String: %s\r\n",jsonReader.value());
                    break;
                case JsonReader::Number:
                    printf("Number: %s\r\n",jsonReader.value());
                    break;
                case JsonReader::Boolean:
                    printf("Boolean: %s\r\n",jsonReader.value());
                    break;
                case JsonReader::Null:
                    printf("Null: (null)\r\n");
                    break;
            }
            break;
        case JsonReader::Field:
            printf("Field: %s\r\n",jsonReader.value());
            break;
        case JsonReader::Array:
            printf("Array: (Start)\r\n");
            break;
        case JsonReader::EndArray:
            printf("Array: (End)\r\n");
            break;
        case JsonReader::Object:
            printf("Object: (Start)\r\n");
            break;
        case JsonReader::EndObject:
            printf("Object: (End)\r\n");
            break;
    }
    printf("Location - Object Depth %lu, Line: %lu, Column %lu, Position %llu\r\n",jsonReader.objectDepth(),jsonReader.context().line(),jsonReader.context().column(),jsonReader.context().position());
}