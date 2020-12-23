// this file isn't used for Arduino projects
#ifndef ARDUINO
#include <stdio.h>
// for perf
#include <chrono>
using namespace std::chrono;
#include "src/ProfilingLexContext.h"
#include "src/MemoryPool.h"
#include "src/JsonReader.h"

void showFileNodes();
void showSkipVsRead();
void showProductionCompanies();
void showShowName();
void showSeasonIndex2EpisodeIndex2Name();
void showNetworksArray();
void showShowExtraction(bool load=false);
void showEpisodes();
void dumpExtraction(const JsonExtractor& extraction,int depth=0);

// this lex context is suitable for profiling 
// and implements a fixed size capture buffer and file input source
// 2kB is usually enough to handle things like overviews or descriptions
// The test file here's longest value clocks in at like 2033 bytes
// Keep in mind we don't to examine every field. If we only care about
// names for example, this can be significantly smaller
ProfilingStaticFileLexContext<2048> fileLC;

// make it HUGE! for testing, as long as we're on a PC. 
// We can always profile and trim it.
StaticMemoryPool<1024*1024> pool;

int main(int argc, char** argv) {
    //showFileNodes();
    //showSkipVsRead();
    showEpisodes();
    printf("\r\n");
    showShowName();
    printf("\r\n");
    showProductionCompanies();
    printf("\r\n");
    showSeasonIndex2EpisodeIndex2Name();
    printf("\r\n");
    //showNetworksArray();
    showShowExtraction(false);
    //showShowExtraction(true);
    
}
void showShowExtraction(bool load) {
    pool.freeAll();
    fileLC.resetUsed();

    // we want name and credit_id from the created_by array's objects
    JsonElement creditName;
    JsonElement creditProfilePath;
    // we also want id, name, and number_of_episodes to air from the root object
    JsonElement id;
    JsonElement name;
    JsonElement numberOfEpisodes;
    // we want the last episode to air's name.
    JsonElement lastEpisodeToAirName;
    
    // create nested the extraction query for $.created_by.name and $.created_by.profile_path
    const char* createdByFields[] = {"name","profile_path"}; // misspell profile_path deliberately to demonstrate <no result> 
    JsonExtractor createdByExtractions[] = {JsonExtractor(&creditName),JsonExtractor(&creditProfilePath)};
    JsonExtractor createdByExtraction(createdByFields,2,createdByExtractions);
    // we want the first index of the created_by array: $.created_by[0]
    JsonExtractor createdByArrayExtractions[] {createdByExtraction};
    size_t createdByArrayIndices[] = {0};
    JsonExtractor createdByArrayExtraction(createdByArrayIndices,1,createdByArrayExtractions);
    // we want the name off of last_episode_to_air, like $.last_episode_to_air.name
    const char* lastEpisodeFields[] = {"name"};
    JsonExtractor lastEpisodeExtractions[] = {JsonExtractor(&lastEpisodeToAirName)};
    JsonExtractor lastEpisodeExtraction(lastEpisodeFields,1,lastEpisodeExtractions);
    // we want id,name, and created by from the root
    // $.id, $.name, $.created_by, $.number_of_episodes and $.last_episode_to_air
    const char* showFields[] = {"id","name","created_by","number_of_episodes","last_episode_to_air"};
    JsonExtractor showExtractions[] = {
        JsonExtractor(&id),
        JsonExtractor(&name),
        createdByArrayExtraction,
        JsonExtractor(&numberOfEpisodes),
        lastEpisodeExtraction
    };
    JsonExtractor showExtraction(showFields,5,showExtractions);
    
    if (!fileLC.open("./data.json"))
    {
        printf("Json file not found\r\n");
        return;
    }
    JsonReader jsonReader(fileLC);
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    if(load) {
        JsonElement je;
        if(jsonReader.parseSubtree(pool,&je)) {
            
            if(je.extract(showExtraction)) {
                dumpExtraction(showExtraction,0);
                milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()); 
                printf("Parsed %llu characters in %d milliseconds using %d bytes of LexContext and %d bytes of the pool.\r\n",fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used(),(int)pool.used());
            } else 
                printf("extract() failed.\r\n");
        }    
    } else {
        if(jsonReader.extract(pool,showExtraction)) {
            dumpExtraction(showExtraction,0);
            milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()); 
            printf("Parsed %llu characters in %d milliseconds using %d bytes of LexContext and %d bytes of the pool.\r\n",fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used(),(int)pool.used());
        } else 
            printf("extract() failed.\r\n");
    }
    if(jsonReader.hasError()) {
        printf("Error: (%d) %s\r\n",(int)jsonReader.lastError(),jsonReader.value());
    }
    fileLC.close();

}
void showEpisodes() {
    pool.freeAll();
    fileLC.resetUsed();
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    JsonElement seasonNumber;
    JsonElement episodeNumber;
    JsonElement name;
    const char* fields[] = {"season_number","episode_number","name"};
    JsonExtractor children[] = {JsonExtractor(&seasonNumber),JsonExtractor(&episodeNumber),JsonExtractor(&name)};
    JsonExtractor extraction(fields,3,children);
    JsonReader jsonReader(fileLC);
    size_t maxUsedPool = 0;
    size_t episodes = 0;
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // JSONPath would be $..episodes[*].name,season_number,episode_number
    while(jsonReader.skipToFieldValue("episodes",JsonReader::Forward)) {
        while(!jsonReader.hasError() && JsonReader::EndArray!=jsonReader.nodeType()) {
            // read the next array element
            if(!jsonReader.read()) 
                break;
            // we keep track of the max pool we use
            if(pool.used()>maxUsedPool)
                maxUsedPool = pool.used();
            // we don't need to keep the pool data between calls here
            pool.freeAll();
            if(!jsonReader.extract(pool,extraction))
                break;
            ++episodes;
            printf("S%02dE%02d %s\r\n",
                (int)seasonNumber.integer(),
                (int)episodeNumber.integer(),
                name.string());
        }
    }
    if(jsonReader.hasError()) {
        // report the error
        printf("\r\nError (%d): %s\r\n\r\n",jsonReader.lastError(),jsonReader.value());
        return;
    } else {
        // stop the "timer"
        milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
        // report the statistics
        printf("Scanned %d episodes and %llu characters in %d milliseconds using %d bytes of LexContext and %d bytes of the pool\r\n",(int)episodes,fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used(),(int)maxUsedPool);
    }
    // always close the file
    fileLC.close();
}
void showNetworksArray() {
    pool.freeAll();
    fileLC.resetUsed();
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    JsonReader jsonReader(fileLC);
    
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    JsonElement je;
    // JSONPath would be $.networks
    if(jsonReader.skipToFieldValue("networks",JsonReader::Siblings)) {
        // load the current document location as an element
        // note that we need pool memory for this
        if(jsonReader.parseSubtree(pool,&je)) {
            printf("Error: (%d) %s\r\n",(int)jsonReader.lastError(),jsonReader.value());    
            return;
        }
    }
    milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()); 
    size_t used = pool.used(); 
    if(!je.undefined()) {
        // note we need pool memory for this to, but don't count it toward the totals in the stats:
        printf("%s\r\n",je.toString(pool));
        printf("Parsed %llu characters in %d milliseconds using %d bytes of LexContext and %d bytes of the pool.\r\n",fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used(),(int)used);
    } else if(jsonReader.hasError()) {
        printf("Error: (%d) %s\r\n",(int)jsonReader.lastError(),jsonReader.value());
    }
    fileLC.close();
}
void showSkipVsRead() {
    fileLC.resetUsed();
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    JsonReader jsonReader(fileLC);
    
    // first profile reading
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    while(jsonReader.read());
    milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    printf("Read %llu characters in %d milliseconds using %d bytes of LexContext\r\n",fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used());
    fileLC.close();
    
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    jsonReader.reset(fileLC);
    fileLC.resetUsed();
    // now profile skipping
    start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    jsonReader.skipSubtree();
    end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    printf("Skipped %llu characters in %d milliseconds using %d bytes of LexContext\r\n",fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used());
    fileLC.close();
}
void showSeasonIndex2EpisodeIndex2Name() {
    fileLC.resetUsed();
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    JsonReader jsonReader(fileLC);
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // JSONPath would be $.seasons[2].episodes[2].name
    if(jsonReader.skipToFieldValue("seasons",JsonReader::Siblings) ) {
        if(jsonReader.skipToIndex(2)) {
            if(jsonReader.skipToFieldValue("episodes",JsonReader::Siblings) ) {
                if(jsonReader.skipToIndex(2)) {
                    if(jsonReader.skipToFieldValue("name",JsonReader::Siblings)) {
                        printf("%s\r\n",jsonReader.value());
                        milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                        printf("Scanned %llu characters in %d milliseconds using %d bytes of LexContext\r\n",fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used());
                    }
                }
            }
        }
    }
    if(jsonReader.hasError()) {
        printf("Error: (%d) %s\r\n",(int)jsonReader.lastError(),jsonReader.value());
    }
    fileLC.close();
}
void showFileNodes() {
    fileLC.resetUsed();
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
                printf("%s\r\n", jsonReader.value()); // print it
                break;
            case JsonReader::Real:                                 // a number!
                printf("Real: %f\r\n", jsonReader.realValue()); // print it
                break;
            case JsonReader::Integer:                                 // a number!
                printf("Integer: %lli\r\n", jsonReader.integerValue()); // print it
                break;
            case JsonReader::Boolean: // a boolean!
                printf("Boolean: %s\r\n", jsonReader.booleanValue() ? "true" : "false");
                break;
            case JsonReader::Null: // a null!
                printf("Null: (null)\r\n");
                break;
            default:
                printf("Undefined!\r\n");
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
    printf("Scanned %lli nodes and %llu characters in %d milliseconds using %d bytes of LexContext\r\n",nodes,fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used());
    fileLC.close();
}
void showProductionCompanies() {
    fileLC.resetUsed();
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
    // JSONPath would be $.production_companies..name
    // find the production_companies field on this level of the hierarchy, than read to its value
    if (jsonReader.skipToFieldValue("production_companies",JsonReader::Siblings) ) {
        // we want only the names that are under the current element. we have to pass a depth tracker
        // to tell the reader where we are.
        unsigned long int depth=0;
        while (jsonReader.skipToFieldValue("name", JsonReader::Descendants,&depth)) {
            ++count;
            printf("%s\r\n",jsonReader.value());
        }
    } 
    // stop the "timer"
    milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // dump the results
    printf("Scanned %llu characters and found %d companies in %d milliseconds using %d bytes of LexContext\r\n",fileLC.position()+1,(int)count, (int)(end.count()-start.count()),(int)fileLC.used());

    // always close the file
    fileLC.close();
}
void showShowName() {
    fileLC.resetUsed();
    // open the file
    if (!fileLC.open("./data.json")) {
        printf("Json file not found\r\n");
        return;
    }
    // create the reader
    JsonReader jsonReader(fileLC);
    
    // time it, for fun.
    milliseconds start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // JSONPath would be $.name
    // we start on nodeType() Initial, but that's sort of a pass through node. skipToField()
    // simply reads through when it finds it, so our first level is actually the root object
    // look for the field named "name" on this level of the heirarchy.
    // then read its value which comes right after it.
    if (jsonReader.skipToFieldValue("name",JsonReader::Siblings)) {
        // deescape the string
        printf("%s\r\n",jsonReader.value());
    } else if(JsonReader::Error==jsonReader.nodeType()) {
        // if we error out, print it
        printf("\r\nError (%d): %s\r\n\r\n",jsonReader.lastError(),jsonReader.value());
        return;
    }
    // stop the "timer"
    milliseconds end = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    // print the results
    printf("Scanned %llu characters in %d milliseconds using %d bytes of LexContext\r\n",fileLC.position()+1,(int)(end.count()-start.count()),(int)fileLC.used());
    // close the file
    fileLC.close();
}
void dumpExtraction(const JsonExtractor& extraction,int depth) {
    if(0==extraction.count) {
        if(nullptr==extraction.presult) {
            printf("(#error NULL result)\r\n");
            return;
        }
        switch(extraction.presult->type()) {
            case JsonElement::Null:
                printf("<null>\r\n");
                break;

            case JsonElement::String:
                printf("%s\r\n",extraction.presult->string());
                break;
            
            case JsonElement::Real:
                printf("%f\r\n",extraction.presult->real());
                break;
            
            case JsonElement::Integer:
                printf("%lli\r\n",extraction.presult->integer());
                break;

            case JsonReader::Boolean:
                printf("%s\r\n",extraction.presult->boolean()?"true":"false");
                break;
            case JsonElement::Undefined:
                printf("<no result>\r\n");
                break;
            
            default:
                printf("%s\r\n",extraction.presult->toString(pool));
                break;
        }
        
        return;
    }
    if(nullptr!=extraction.pfields) {    
        if(0<extraction.count) {
            //for(int j = 0;j<depth;++j)
            //    printf("  ");
            JsonExtractor q=extraction.pchildren[0];
            printf("%s: ",extraction.pfields[0]);
            dumpExtraction(extraction.pchildren[0],depth+1);
        }
        for(size_t i = 1;i<extraction.count;++i) {
            JsonExtractor q=extraction.pchildren[i];
            for(int j = 0;j<depth;++j)
                printf("  ");
            printf("%s: ",extraction.pfields[i]);
            dumpExtraction(extraction.pchildren[i],depth+1);
        }
    } else if(nullptr!=extraction.pindices) {
        if(0<extraction.count) {
            //for(int j = 0;j<depth;++j)
            //    printf("  ");
            JsonExtractor q=extraction.pchildren[0];
            printf("[%lli]: ",(long long int)extraction.pindices[0]);
            dumpExtraction(extraction.pchildren[0],depth+1);
        }
        for(size_t i = 1;i<extraction.count;++i) {
            JsonExtractor q=extraction.pchildren[i];
            for(int j = 0;j<depth;++j)
                printf("  ");
            printf("[%lli]: ",(long long int)extraction.pindices[i]);
            dumpExtraction(extraction.pchildren[i],depth+1);
        }
    }
}
#endif