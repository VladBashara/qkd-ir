#include <filesystem>
#include <cxxopts.hpp>
#include "logger.h"

int main(int argc, char* argv[])
{
    try
    {
        cxxopts::Options options("logger", "Sending and writing logs");
        options.add_options()
            ("h,help", "Help info", cxxopts::value<bool>()->default_value("false"))
            ("l,log-file", "File for logs", cxxopts::value<std::string>())
            ("d,data-base-file", "File name and path for data base", cxxopts::value<std::string>())
            ("gmin,generator-port-min", "Min port for generator", cxxopts::value<int>())
            ("gmax,generator-port-max", "Max port for generator", cxxopts::value<int>())
            ("c,client-port", "Port for client", cxxopts::value<int>())
            ;
        auto result = options.parse(argc, argv);

        if (result.arguments().size() == 0 or result["h"].count() > 0)
        {
            std::cout << options.help() << std::endl;
            return 0;
        }

        std::string filename = result["l"].as<std::string>();
        std::string dbfilename = result["d"].as<std::string>();
        int generatorPortMin = result["gmin"].as<int>();
        int generatorPortMax = result["gmax"].as<int>();
        int clientPort = result["c"].as<int>();

        Logger logger(generatorPortMin, generatorPortMax, clientPort, filename, dbfilename);
        logger.connectGenerators();
        logger.connectClient();

        std::this_thread::sleep_for(std::chrono::hours(1));

        logger.stop();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

