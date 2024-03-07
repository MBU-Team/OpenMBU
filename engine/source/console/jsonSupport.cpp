#include "console/simBase.h"
#include "arrayObject.h"
#include <json/json.h>
#include <cmath>
#include <sstream>
#include <string>
#include <limits>
#ifdef _WIN32
#include <Shlwapi.h>
#define strcasecmp _stricmp
#define strcasestr StrStrI
#else
#include <strings.h>
#endif

const char* toString(Json::Value& value);

class JSONObject : public SimObject
{
    typedef SimObject Parent;
public:

    DECLARE_CONOBJECT(JSONObject);
};

IMPLEMENT_CONOBJECT(JSONObject);


SimObject* getJSONObject(Json::Value& value) {
    SimObject* obj = new JSONObject();
    obj->registerObject();

    //Get all the sub objects
    for (Json::ValueIterator it = value.begin(); it != value.end(); it++) {
        Json::Value key = it.key();
        Json::Value val = *it;

        const char* value = toString(val);
        obj->setDataField(StringTable->insert(key.asCString(), false), NULL, StringTable->insert(value, true));
        if (it->type() == Json::objectValue || it->type() == Json::arrayValue) {
            obj->setDataField("__obj"_ts, StringTable->insert(key.asCString(), false), "true"_ts);
        }
    }

    return obj;
}

ArrayObject* getJSONArray(Json::Value& value) {
    ArrayObject* array = new ArrayObject();
    array->registerObject();

    for (U32 i = 0; i < value.size(); i++) {
        Json::Value val = value[i];

        array->addEntry(toString(val));
        if (val.type() == Json::objectValue || val.type() == Json::arrayValue) {
            char* retBuf = Con::getReturnBuffer(1024);
            dSprintf(retBuf, 1024, "%d", i);
            array->setDataField("__obj"_ts, StringTable->insert(retBuf, false), "true"_ts);
        }
    }

    return array;
}

const char* toString(Json::Value& value) {
    char* retBuf = Con::getReturnBuffer(1024);
    switch (value.type()) {
    case Json::objectValue: {
        SimObject* obj = getJSONObject(value);
        return obj->getIdString();
    }
    case Json::stringValue: {
        const char* str = value.asCString();
        //Copy to a torque stack string so we don't have to worry about memory
        char* tstr = Con::getReturnBuffer(dStrlen(str) + 1);
        dStrcpy(tstr, str);
        return tstr;
    }
    case Json::intValue:
        dSprintf(retBuf, 1024, "%lld", value.asInt64());
        return retBuf;
    case Json::realValue:
        dSprintf(retBuf, 1024, "%f", value.asDouble());
        return retBuf;
    case Json::uintValue:
        dSprintf(retBuf, 1024, "%llu", value.asUInt64());
        return retBuf;
    case Json::booleanValue:
        return value.asBool() ? "true" : "false";
    case Json::nullValue:
        return "";
    case Json::arrayValue: {
        SimObject* obj = getJSONArray(value);
        return obj->getIdString();
    }
    default:
        return "";
    }
}

ConsoleFunction(jsonParse, const char*, 2, 2, "jsonParse(string json);") {
    const char* json = argv[1];
    if (*json == 0) {
        //Empty string passed, don't even bother
        return "";
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();

    std::string errs;
    if (reader->parse(json, json + dStrlen(json), &root, &errs)) {
        delete reader;
        return toString(root);
    }
    else {
        delete reader;
        Con::errorf("JSON Parse error: %s", errs.c_str());
        return "";
    }
}

bool toJson(const char* input, bool expandObject, Json::Value& output) {
    S32 length = dStrlen(input);
    if (length == 0) {
        //Null
        output = Json::Value(Json::nullValue);
        return true;
    }

    SimObject* obj = Sim::findObject(input);
    if (expandObject && obj) {
        //Object of some sort
        const char* className = obj->getClassRep()->getClassName();
        bool isJsonObject = dynamic_cast<JSONObject*>(obj) != nullptr;
        //Make sure we only do this to ScriptObjects and Arrays. Could probably
        // expand this to add support for generic torque objects but cbf right now
        if (dStrcmp(className, "ScriptObject") == 0 || isJsonObject) {
            //Generic object
            output = Json::Value(Json::objectValue);

            //Member fields of the object
            AbstractClassRep::FieldList& list = obj->getClassRep()->mFieldList;

            //List all dynamic fields
            SimFieldDictionaryIterator itr(obj->getFieldDictionary());

            SimFieldDictionary::Entry* walk;
            while ((walk = *itr) != NULL)
            {
                //Don't include any dynamic fields with the same name as a member field
                S32 j = 0;
                for (j = 0; j < list.size(); j++)
                    if (list[j].pFieldname == walk->slotName)
                        break;
                //If we didn't get to the end of the list then we hit one
                if (j != list.size()) {
                    ++itr;
                    continue;
                }

                const char* key = walk->slotName;
                const char* value = walk->value;

                if (dStrlen(key) > 2 && key[0] == '_' && key[1] == '_') {
                    ++itr;
                    continue;
                }

                bool expand = dAtob(obj->getDataField("__obj"_ts, StringTable->insert(key, false)));

                //Insert into the array
                Json::Value val;
                if (!toJson(value, expand, val))
                    return false;
                output[key] = val;

                ++itr;
            }
            //Just has no fields
            return true;
        }
        ArrayObject* array = dynamic_cast<ArrayObject*>(obj); // Its an array instead!
        if (array != nullptr)
        {
            S32 size = array->getSize();
            for (S32 i = 0; i < size; i++) {
                std::string value = array->getEntry(i);
                char* retBuf = Con::getReturnBuffer(1024);
                dSprintf(retBuf, 1024, "%d", i);
                bool expand = dAtob(obj->getDataField("__obj"_ts, StringTable->insert(retBuf, false)));

                //Insert into the array
                Json::Value val;
                if (!toJson(value.c_str(), expand, val))
                    return false;
                output[i] = val;
            }

            return true;
        }
        // Fallback to regular string/number
    }

    //Not an object, try some special things
    if (strcasecmp(input, "true") == 0 || strcasecmp(input, "false") == 0) {
        //It's a boolean
        output = Json::Value(dAtob(input));
        return true;
    }

    //Special things
#define specialCase(search, value) \
	if (strcasecmp(input, search) == 0) { \
		output = Json::Value(value); \
		return true; \
	}

    specialCase("inf", std::numeric_limits<F32>::infinity());
    specialCase("-inf", -std::numeric_limits<F32>::infinity());
    specialCase("1.#inf", std::numeric_limits<F32>::infinity());
    specialCase("-1.#inf", -std::numeric_limits<F32>::infinity());
    specialCase("nan", std::numeric_limits<F32>::quiet_NaN());
    specialCase("-nan", -std::numeric_limits<F32>::quiet_NaN());
    specialCase("1.#qnan", std::numeric_limits<F32>::quiet_NaN());
    specialCase("-1.#ind", -std::numeric_limits<F32>::quiet_NaN());

#undef specialCase

    //At this point it's either a number or a string
    //Easy to check for if it's a number
    const char* numberChars = "0123456789.-+eE";

    for (S32 i = 0; i < length; i++) {
        if (dStrchr(numberChars, input[i]) == NULL) {
            //It's a string
            output = Json::Value((const char*)input);
            return true;
        }
    }

    //It's a number. Check if it's floating
    if (dStrchr(input, '.') != NULL || dStrchr(input, 'e') != NULL) {
        //It's a float
        output = Json::Value(dAtof(input));
        return true;
    }

    //Only thing left to be is an integer
    output = Json::Value(dAtoi(input));
    return true;
}

ConsoleFunction(jsonPrint, const char*, 2, 2, "jsonPrint(value);") {
    const char* input = argv[1];

    //Try to parse the input into a JSON object
    Json::Value val;
    if (!toJson(input, true, val)) {
        Con::errorf("Error printing json: could not parse!");
        return "";
    }
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    Json::StreamWriter* writer = builder.newStreamWriter();

    //Write it out to a string format
    std::stringstream ss;
    if (writer->write(val, &ss) == 0 && ss.good()) {
        //Convert it to something Torque can handle
        std::string str = ss.str();
        char* buffer = Con::getReturnBuffer(str.length() + 1);
        dStrcpy(buffer, str.c_str());
        delete writer;
        return buffer;
    }
    delete writer;

    Con::errorf("Error printing json: could not write!");
    return "";
}

