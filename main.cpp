#include "json.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h" 
#include "json_reader.h"
#include "svg.h"

int main() {
        using namespace svg;
        using namespace std;
        
        TransportCatalogue cat;
        //cat.GetStops();
        //cat.GetBuses();
        Text text;
        Jsonreader reader;
        const json::Document doc_ = json::Load(cin);
        
        const auto& root = doc_.GetRoot().AsMap();
        try {
            reader.JsonReaderStdin(doc_, cat); // catalogue
        }
        catch (out_of_range& e) {
            cerr << "Troubles in JsonReaderStdin " << e.what() << endl;
        }
        auto it = root.find("render_settings");

        if (it == root.end()) {
        
            cerr << "render_settings not found in root\n";
            return 0;
        }
        const auto& render_settings_json = it->second;

        MapRenderer::RenderSettings render_settings =
        MapRenderer::ParseRenderSettingsFromJson(render_settings_json);
        
        MapRenderer map_renderer(render_settings);
        
        map_renderer.SetRoute(cat);
        map_renderer.RenderMap(cat);
        //svg_document.Add(Polyline().AddPoint);
       // doc_.Render(cout);
        //svg_document.Render(cout);
        //p.Render(cout);
        
        return 0;

        //Jsonreader reader;
        //const auto& doc = json::Load(cin);
        //TransportCatalogue catalogue;
        //MapRenderer::RenderSettings settings = MapRenderer::ParseRenderSettingsFromJson(render_settings_json);
        //MapRenderer renderer(settings);
        //RequestHandler handler(catalogue, renderer);
        //reader.JsonReaderStdin(handler);
        //svg::Document map = handler.RenderMap(catalogue);
        //map.Render(std::cout);
}