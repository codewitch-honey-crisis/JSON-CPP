// only use this project for Arduino
#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#include "src/ProfilingLexContext.h"
#include "src/MemoryPool.h"
#include "src/JsonReader.h"
void setup();
void loop();
void showFileNodes();
void showSkipVsRead();
void showProductionCompanies();
void showShowName();
void showSeasonIndex2EpisodeIndex2Name();
void showNetworksArray();
void showShowExtraction(bool load=false);
void showEpisodes();
void dumpExtraction(const JsonExtractor& extraction,int depth=0);

ProfilingArduinoLexContext<256> fileLC;
StaticMemoryPool<256> pool;

void loop() {}
void setup() {
    // wire an SD reader to your 
    // first SPI bus and the standard 
    // CS for your board
    //
    // put data.json on it but
    // name it "data.jso" due
    // to 8.3 filename limits
    // on some platforms
    Serial.begin(115200);
     
    if(!SD.begin()) {
      Serial.println("Unable to mount SD");
      while(true); // halt
    }
    
    showEpisodes();
    Serial.println();
    showShowName();
    Serial.println();
    showProductionCompanies();
    Serial.println();
    showSeasonIndex2EpisodeIndex2Name();
    Serial.println();
    //showNetworksArray();
    //Serial.println();
    showShowExtraction(false);
    //Serial.println();
    //showShowExtraction(true);
    
}
void showShowExtraction(bool load) {
    pool.freeAll();
    fileLC.resetUsed();
    // create nested the extraction query

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
    const char* createdByFields[] = {"name","profile_path"};
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


    File file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    JsonReader jsonReader(fileLC);
    uint32_t start = millis();
    if(load) {
        JsonElement e;
        if(jsonReader.parseSubtree(pool,&e)) {
            
            if(e.extract(showExtraction)) {
                dumpExtraction(showExtraction,0);
                uint32_t end = millis();
                Serial.print(F("Parsed "));
                Serial.print((int)fileLC.position()+1);
                Serial.print(F(" characters in "));
                Serial.print(end-start);
                Serial.print(F(" milliseconds using "));
                Serial.print((int)fileLC.used());
                Serial.print(F(" bytes of LexContext and "));
                Serial.print((int)pool.used());
                Serial.println(F(" bytes of the pool"));
            } else 
                Serial.println("extract() failed.");
        }    
    } else {
        if(jsonReader.extract(pool,showExtraction)) {
            dumpExtraction(showExtraction,0);
            uint32_t end = millis();
            Serial.print(F("Parsed "));
            Serial.print((int)fileLC.position()+1);
            Serial.print(F(" characters in "));
            Serial.print(end-start);
            Serial.print(F(" milliseconds using "));
            Serial.print((int)fileLC.used());
            Serial.print(F(" bytes of LexContext and "));
            Serial.print((int)pool.used());
            Serial.println(F(" bytes of the pool"));
        } else 
            Serial.println("extract() failed.");
        
    }
    file.close();
}
void showEpisodes() {
    pool.freeAll();
    fileLC.resetUsed();
    File file = SD.open("/data.jso","r");
    if(NULL==file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    JsonElement seasonNumber;
    JsonElement episodeNumber;
    JsonElement name;
    const char* fields[] = {"season_number","episode_number","name"};
    JsonExtractor subqueries[] = {JsonExtractor(&seasonNumber),JsonExtractor(&episodeNumber),JsonExtractor(&name)};
    JsonExtractor extraction(fields,3,subqueries);
    JsonReader jsonReader(fileLC);
    size_t maxUsedPool = 0;
    size_t episodes = 0;

    uint32_t start = millis();

    // JSONPath would be $..episodes:
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
            Serial.print(F("S"));
            if(seasonNumber.integer()<10)
              Serial.print(F("0"));
            Serial.print((int)seasonNumber.integer());
            Serial.print(F("E"));
            if(episodeNumber.integer()<10)
              Serial.print(F("0"));
            Serial.print((int)episodeNumber.integer());
            Serial.print(" ");
            Serial.println(name.string());
        }
    }
    if(jsonReader.hasError()) {
        // report the error
        Serial.print(F("Error: ("));
        Serial.print((int)jsonReader.lastError());
        Serial.print(F(") "));
        Serial.println(jsonReader.value());
    } else {
        // stop the "timer"
        // and 
        // report the statistics        
        uint32_t end = millis();
        Serial.print(F("Scanned "));
        Serial.print((int)episodes);
        Serial.print(F(" episodes and "));
        // overflows on small word sizes
        Serial.print((int)fileLC.position()+1);
        Serial.print(F(" characters in "));
        Serial.print(end-start);
        Serial.print(F(" milliseconds using "));
        Serial.print((unsigned int)fileLC.used());
        Serial.print(F(" bytes of LexContext and "));
        Serial.print((unsigned int)maxUsedPool);
        Serial.println(" bytes of the pool");
        
    }
    file.close();

}
void showNetworksArray() {
    pool.freeAll();
    fileLC.resetUsed();
    File file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    JsonReader jsonReader(fileLC);
    uint32_t start = millis();
    JsonElement elem;
    // JSONPath would be $.networks
    if(jsonReader.skipToFieldValue("networks",JsonReader::Siblings)) {
        // load the current document location as an element
        // note that we need pool memory for this
        jsonReader.parseSubtree(pool,&elem);
    }
    uint32_t end = millis();
    size_t used = pool.used(); 
    if(!elem.undefined()) {
        // note we need pool memory for this to, but don't count it toward the totals in the stats:
        uint32_t end = millis();
        Serial.print(F("Parsed "));
        Serial.print((int)fileLC.position()+1);
        Serial.print(F(" characters in "));
        Serial.print(end-start);
        Serial.print(F(" milliseconds using "));
        Serial.print((int)fileLC.used());
        Serial.print(F(" of LexContext and "));
        Serial.print((int)used);
        Serial.println(F(" bytes of the pool"));
    } else if(jsonReader.hasError()) {
        Serial.print(F("Error: ("));
        Serial.print((int)jsonReader.lastError());
        Serial.print(F(") "));
        Serial.println(jsonReader.value());
    }
    file.close();
}
void showSkipVsRead() {
    fileLC.resetUsed();
    File file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    JsonReader jsonReader(fileLC);
    
    // first profile reading
    uint32_t start = millis();
    while(jsonReader.read());
    uint32_t end = millis();
    Serial.print(F("Read "));
    Serial.print((int)fileLC.position()+1);
    Serial.print(F(" characters in "));
    Serial.print(end-start);
    Serial.print(F(" milliseconds using "));
    Serial.print(fileLC.used());
    Serial.println(F(" of LexContext"));
    file.close();

    file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);

    jsonReader.reset(fileLC);
    fileLC.resetUsed();
    // now profile skipping
    start = millis();
    jsonReader.skipSubtree();
    end = millis();
    Serial.print(F("Skipped "));
    Serial.print((int)fileLC.position()+1);
    Serial.print(F(" characters in "));
    Serial.print(end-start);
    Serial.print(F(" milliseconds using "));
    Serial.print(fileLC.used());
    Serial.println(F(" of LexContext"));
    end = millis();
    file.close();
}
void showSeasonIndex2EpisodeIndex2Name() {
    fileLC.resetUsed();
    File file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    JsonReader jsonReader(fileLC);
    uint32_t start = millis();
    // JSONPath would be $.seasons[2].episodes[2].name
    if(jsonReader.skipToFieldValue("seasons",JsonReader::Siblings) ) {
        if(jsonReader.skipToIndex(2)) {
            if(jsonReader.skipToFieldValue("episodes",JsonReader::Siblings) ) {
                if(jsonReader.skipToIndex(2)) {
                    if(jsonReader.skipToFieldValue("name",JsonReader::Siblings)) {
                        Serial.println(jsonReader.value());
                        uint32_t end = millis();
                        Serial.print(F("Scanned "));
                        Serial.print((int)fileLC.position()+1);
                        Serial.print(F(" characters in "));
                        Serial.print(end-start);
                        Serial.print(F(" milliseconds using "));
                        Serial.print((int)fileLC.used());
                        Serial.println(F(" bytes of LexContext"));
                    }
                }
            }
        }
    }
    if(jsonReader.hasError()) {
      Serial.print(F("Error: ("));
      Serial.print((int)jsonReader.lastError());
      Serial.print(F(") "));
      Serial.println(jsonReader.value());
    }
    file.close();
}
void showFileNodes() {
    fileLC.resetUsed();
    File file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    JsonReader jsonReader(fileLC);
    long long int nodes = 0; // we don't count the initial node
    uint32_t start = millis();
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
    uint32_t end = millis();
    Serial.print(F("Read "));
    Serial.print((int)nodes);
    Serial.print(F(" nodes and "));
    Serial.print((int)fileLC.position()+1);
    Serial.print(F(" characters in "));
    Serial.print(end-start);
    Serial.print(F(" milliseconds using "));
    Serial.print(fileLC.used());
    Serial.println(F(" bytes of LexContext"));
    file.close();

}
void showProductionCompanies() {
    fileLC.resetUsed();
    File file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    // create the reader
    JsonReader jsonReader(fileLC);
    // keep track of the number we find
    size_t count = 0;
    // time it for fun
    uint32_t start = millis();
    // JSONPath would be $.production_companies..name
    // find the production_companies field on this level of the hierarchy, than read to its value
    if (jsonReader.skipToFieldValue("production_companies",JsonReader::Siblings) ) {
        // we want only the names that are under the current element. we have to pass a depth tracker
        // to tell the reader where we are.
        unsigned long int depth=0;
        while (jsonReader.skipToFieldValue("name", JsonReader::Descendants,&depth)) {
            ++count;
            Serial.println(jsonReader.value());
        }
    } 
    // stop the "timer"
    uint32_t end = millis();
    Serial.print(F("Scanned "));
    Serial.print((int)fileLC.position()+1);
    Serial.print(F(" characters and found "));
    Serial.print(count);
    Serial.print(F(" companies in "));
    Serial.print(end-start);
    Serial.print(F(" milliseconds using "));
    Serial.print((int)fileLC.used());
    Serial.println(F(" bytes of LexContext"));
    file.close();
}
void showShowName() {
    File file = SD.open("/data.jso","r");
    if(!file) {
      Serial.println(F("Json file not found"));
      return;
    }
    fileLC.begin(file);
    // create the reader
    JsonReader jsonReader(fileLC);
    
    // time it, for fun.
    uint32_t start = millis();
    // JSONPath would be $.name
    // we start on nodeType() Initial, but that's sort of a pass through node. skipToField()
    // simply reads through when it finds it, so our first level is actually the root object
    // look for the field named "name" on this level of the heirarchy.
    // then read its value which comes right after it.
    if ( jsonReader.skipToFieldValue("name",JsonReader::Siblings)) {
        Serial.println(jsonReader.value());
    } else if(jsonReader.hasError()) {
        // if we error out, print it
        Serial.print(F("Error ("));
        Serial.print(jsonReader.lastError());
        Serial.print(F(") "));
        Serial.println(jsonReader.value());
        return;
    }
    // stop the "timer"
    uint32_t end = millis();
    Serial.print(F("Scanned "));
    Serial.print((unsigned int)fileLC.position()+1);    
    Serial.print(F(" characters in "));
    Serial.print(end-start);
    Serial.print(F(" milliseconds using "));
    Serial.print(fileLC.used());
    Serial.println(" bytes");
    file.close();    
}
void dumpExtraction(const JsonExtractor& extraction,int depth) {
    if(0==extraction.count) {
        if(nullptr==extraction.presult) {
            Serial.println(F("(#error NULL result)"));
            return;
        }
        switch(extraction.presult->type()) {
            case JsonElement::Null:
                Serial.println(F("<null>"));
                break;

            case JsonElement::String:
                Serial.println(extraction.presult->string());
                break;
            
            case JsonElement::Real:
                Serial.println(extraction.presult->real());
                break;
            
            case JsonElement::Integer:
                Serial.println((int)extraction.presult->integer(),DEC);
                break;

            case JsonReader::Boolean:
                Serial.println(extraction.presult->boolean()?F("true"):F("false"));
                break;
            case JsonElement::Undefined:
                Serial.println(F("<no result>"));
                break;
            
            default:
                Serial.println(extraction.presult->toString(pool));
                break;
        }
        
        return;
    }
    if(nullptr!=extraction.pfields) {    
        if(0<extraction.count) {
            JsonExtractor q=extraction.pchildren[0];
            Serial.print(extraction.pfields[0]);
            Serial.print(F(": "));
            dumpExtraction(extraction.pchildren[0],depth+1);
        }
        for(size_t i = 1;i<extraction.count;++i) {
            JsonExtractor q=extraction.pchildren[i];
            for(int j = 0;j<depth;++j)
                Serial.print(F("  "));
            Serial.print(extraction.pfields[i]);
            Serial.print(F(": "));
            dumpExtraction(extraction.pchildren[i],depth+1);
        }
    } else if(nullptr!=extraction.pindices) {
        if(0<extraction.count) {
            Serial.print(F("["));
            Serial.print((int)extraction.pindices[0],DEC);
            Serial.print(F("]: "));
            dumpExtraction(extraction.pchildren[0],depth+1);
        }
        for(size_t i = 1;i<extraction.count;++i) {
            JsonExtractor q=extraction.pchildren[i];
            for(int j = 0;j<depth;++j)
                Serial.print(F("  "));
            Serial.print(F("["));
            Serial.print((int)extraction.pindices[i],DEC);
            Serial.print(F("]: "));
            dumpExtraction(extraction.pchildren[i],depth+1);
        }
    }
}
#endif
