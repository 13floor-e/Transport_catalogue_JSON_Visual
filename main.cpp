#include "json.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h" 
#include "json_reader.h"
#include "svg.h"

using namespace std;

int main() {
    // json::Print(
    //     json::Document{
    //         json::Builder{}
    //         .StartDict()
    //             .Key("key1"s).Value(123)
    //             .Key("key2"s).Value("value2"s)
    //             .Key("key3"s).StartArray()
    //                 .Value(456)
    //                 .StartDict().EndDict()
    //                 .StartDict()
    //                     .Key(""s)
    //                     .Value(nullptr)
    //                 .EndDict()
    //                 .Value(""s)
    //             .EndArray()
    //         .EndDict()
    //         .Build()
    //     },
    //     cout
    // );
    // cout << endl;
    
    // json::Print(
    //     json::Document{
    //         json::Builder{}
    //         .Value("just a string"s)
    //         .Build()
    //     },
    //     cout
    // );
    // cout << endl;

     using namespace svg;
        using namespace std;
        
        const json::Document doc_ = json::Load(cin);

        TransportCatalogue cat;
        //Text text;
        JsonReader reader;
        MapRenderer render;
              
        try {
            reader.ReadBaseRequests(doc_, cat); // catalogue
        }
        catch (out_of_range& e) {
            cerr << "Troubles in JsonReaderStdin " << e.what() << endl;
        }

        cat.GetBuses();
        cat.GetStops();

        const auto& root = doc_.GetRoot().AsMap();

        auto it = root.find("render_settings");

        if (it == root.end()) {
        
            cerr << "render_settings not found in root\n";
            return 0;
        }

        const auto& render_settings_json = it->second;

        RenderSettings render_settings =
        MapRenderer::FromJson(render_settings_json);
        
        MapRenderer map_renderer(render_settings);

        try {
            const json::Node result =
                reader.ReadStatRequests(doc_, cat, map_renderer);

            json::Print(json::Document{ result }, cout);
        }
        catch (out_of_range& e) {
            cerr << "troubles in Pocesses " << e.what() << endl;
        }
        
        return 0;
}