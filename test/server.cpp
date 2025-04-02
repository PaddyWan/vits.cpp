#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>

#include <vits.h>

using json = nlohmann::ordered_json;
using namespace httplib;

//Inspiration for this:
// https://github.com/ggerganov/llama.cpp/blob/master/examples/server/server.cpp
// => https://github.com/yhirose/cpp-httplib
// => https://json.nlohmann.me/
// https://github.com/Artrajz/vits-simple-api
// => https://huggingface.co/spaces/Artrajz/vits-simple-api
// => https://artrajz-vits-simple-api.hf.space/voice/speakers
// https://github.com/SillyTavern/SillyTavern
static std::string safe_json_to_str(json data) {
    return data.dump(-1, ' ', false, json::error_handler_t::replace);
}

struct WAVHeader {
    char riff_header[4];         // Contains "RIFF"
    int wav_size;                // Size of the WAV file
    char wave_header[4];         // Contains "WAVE"
    char fmt_header[4];          // Contains "fmt " (with a space after fmt)
    int fmt_chunk_size;          // Should be 16 for PCM format
    short audio_format;          // Should be 1 for PCM format
    short num_channels;
    int sample_rate;
    int byte_rate;               // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    short sample_alignment;      // num_channels * Bytes Per Sample
    short bit_depth;             // Number of bits per sample
    char data_header[4];         // Contains "data"
    int data_bytes;              // Number of bytes in data. Number of samples * num_channels * sample byte size
};

void write_wav_memory(std::vector<char>& data, float* samples, size_t size, size_t sample_rate=16000) {
    WAVHeader wav_header;
    int num_channels = 1;
    int bit_depth = 16;
    
    std::vector<short> pcm_samples(size);
    for (size_t i = 0; i < size; ++i) 
        pcm_samples[i] = static_cast<short>(std::max(-1.0f, std::min(1.0f, samples[i])) * 32767);

    memcpy(wav_header.riff_header, "RIFF", 4);
    memcpy(wav_header.wave_header, "WAVE", 4);
    memcpy(wav_header.fmt_header, "fmt ", 4);
    wav_header.fmt_chunk_size = 16;
    wav_header.audio_format = 1;
    wav_header.num_channels = num_channels;
    wav_header.sample_rate = sample_rate;
    wav_header.byte_rate = sample_rate * num_channels * (bit_depth / 8);
    wav_header.sample_alignment = num_channels * (bit_depth / 8);
    wav_header.bit_depth = bit_depth;
    memcpy(wav_header.data_header, "data", 4);
    wav_header.data_bytes = pcm_samples.size() * (bit_depth / 8);
    wav_header.wav_size = 4 + (8 + wav_header.fmt_chunk_size) + (8 + wav_header.data_bytes);

    // Write header to file
    data.insert(data.end(), reinterpret_cast<const char*>(&wav_header), reinterpret_cast<const char*>(&wav_header) + sizeof(WAVHeader));
    data.insert(data.end(), reinterpret_cast<const char*>(pcm_samples.data()), reinterpret_cast<const char*>(pcm_samples.data()) + wav_header.data_bytes);
}

struct VITSParams {
    int n_threads                 = -1;
    std::string model_path        = "./scripts/vits-spanish.ggml";
    std::string output_path       = "./output.wav";
    int64_t seed                  = -1;
    bool precut_input             = true;
};

void print_usage(int argc, char** argv) {
    printf("usage: %s [arguments]\n", argv[0]);
    printf("\n");
    printf("arguments:\n");
    printf("  -h, --help                         show this help message and exit\n");
    printf("  -t, --threads N                    number of threads to use during computation (default: -1).\n");
    printf("                                     If threads <= 0, then threads will be set to the number of CPU physical cores\n");
    printf("  -m, --model   MODEL                path to model\n");
    printf("  -o, --output  OUTPUT_PATH          path to write result image to (default: ./output.wav)\n");
    printf("  -s SEED, --seed SEED               RNG seed (default: -1, use random seed for < 0)\n");
    printf("  -c/-C                              Preparse input text, used so the model doesn't need to translate large pieces of text (-c on, -C off, default: on)\n");
}

void parse_args(int argc, char** argv, VITSParams& params) {
    bool invalid_arg = false;
    std::string arg;
    for (int i = 1; i < argc; i++) {
	arg = argv[i];
	if (arg == "-t" || arg == "--threads") {
	    if (++i >= argc) {
		invalid_arg = true;
		break;
	    }
	    params.n_threads = std::stoi(argv[i]);
	} else if (arg == "-m" || arg == "--model") {
	    if (++i >= argc) {
		invalid_arg = true;
		break;
	    }
	    params.model_path = argv[i];
	} else if (arg == "-o" || arg == "--output") {
	    if (++i >= argc) {
                invalid_arg = true;
		break;
	    }
	    params.output_path = argv[i];
	} else if (arg == "-s" || arg == "--seed") {
	    if (++i >= argc) {
		invalid_arg = true;
		break;
	    }
	    params.seed = std::stoll(argv[i]);
	} else if (arg == "-c") {
	    params.precut_input = true;
	} else if (arg == "-C") {
	    params.precut_input = false;
	} else if (arg == "-h" || arg == "--help") {
	    print_usage(argc, argv);
	    exit(0);
	} else {
	    fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
	    print_usage(argc, argv);
	    exit(1);
	}
    }
    if (invalid_arg) {
	fprintf(stderr, "error: invalid parameter for argument: %s\n", arg.c_str());
	print_usage(argc, argv);
	exit(1);
    }
    if (params.n_threads <= 0) {
	unsigned int n_threads = std::thread::hardware_concurrency();
	params.n_threads = (n_threads > 0 ? (n_threads <= 4 ? n_threads : n_threads / 2) : 4);
    }
    if (params.seed < 0) {
	srand((int)time(NULL));
	params.seed = rand();
    }
}

int main(int argc, char** argv)
{
    VITSParams params;
    parse_args(argc, argv, params);

    vits_init_backend(params.n_threads);
    
    vits_model * model = vits_model_load_from_file(params.model_path.c_str());
    assert(model != nullptr);
    rng.seed(params.seed);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLServer svr("./cert.pem", "./key.pem"); //todo
#else
    Server svr;
#endif
    svr.Get("/voice/speakers", [](const Request& req, Response& res) {
	res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin"));
	//json data = json::parse(req.body);
	json speakers = {
	    {"VITS", {
		    {
			{"id",   0}, //currently only support one model
			{"name", "default"},
			{"lang", {"unknown"}}
		    }
		}
	    },
	    {"BERT-VITS2", json::value_t::array}, //required by the API
	    {"W2V2-VITS", json::value_t::array}
	};
	res.set_content(safe_json_to_str(speakers), "application/json; charset=utf-8");
	res.status = 200;
    });
    const auto voicevits = [model,params](const Request& req, Response& res)
    {
	res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin"));
	if(req.has_param("text"))
	{
	    std::string text = req.get_param_value("text");
	    std::vector<float> wavedata;
	    if(params.precut_input)
	    {
		const char* b = text.data();
		const char* e = text.data()+text.size();
		while(b!=e && b[0] == '"')
		    ++b;
		while(b!=e)
		{
		    const std::size_t pos = std::string_view(b,e).find_first_of("!\\,.:;?");
		    std::string subtext;
		    if(pos != std::string::npos)
		    {
			subtext = std::string(b, b+pos+1);
			b+=(pos+1);
		    }
		    else
		    {
			subtext= std::string(b,e);
			b=e;
		    }
		    std::cout << subtext << std::endl;
		    auto result = vits_model_process(model, subtext.c_str());
		    wavedata.insert(wavedata.end(), result.data, result.data + result.size);
		    vits_free_result(result);
		    while(b!=e && b[0] == '"')
			++b;
		}
	    }
	    else
	    {
		auto result = vits_model_process(model, text.c_str());
		wavedata.insert(wavedata.end(), result.data, result.data + result.size);
		vits_free_result(result);
	    }
	    std::vector<char> data;
	    write_wav_memory(data, wavedata.data(), wavedata.size(), model->sample_rate());
	    res.set_content_provider(
		data.size(), // Content length
		"audio/wave", // Content type
		[data](size_t offset, size_t length, DataSink &sink)
		{
		    sink.write(data.data()+offset, length);
		    return true;
		},
		[](bool success) { /*you can delete data here*/ });
	}
	else
	{
	    //todo
	    //std::cout << req.body << std::endl;
	    //json data = json::parse(req.body);
	    //std::cout << data << std::endl;
	}
    };
    svr.Get("/voice/vits", voicevits);
    svr.Post("/voice/vits", voicevits);
    //check gives the same data as id = and model = from speakers
//    svr.Get("/voice/check", [](const Request& req, Response& res) {
//	std::cout << "/voice/check" << req.body << std::endl;
//	std::cout << req.body << std::endl;
//    });
    svr.listen("0.0.0.0", 8002);
    vits_free_model(model);

    vits_free_backend();
    return 0;
}
