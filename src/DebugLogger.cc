#ifdef DEBUG

#include "zeek/DebugLogger.h"

#include <unistd.h>
#include <cstdlib>

#include "zeek/RunState.h"
#include "zeek/plugin/Plugin.h"

zeek::detail::DebugLogger zeek::detail::debug_logger;
zeek::detail::DebugLogger& debug_logger = zeek::detail::debug_logger;

namespace zeek::detail
	{

// Same order here as in DebugStream.
DebugLogger::Stream DebugLogger::streams[NUM_DBGS] = {
	{"serial", 0, false},          {"rules", 0, false},         {"string", 0, false},
	{"notifiers", 0, false},       {"main-loop", 0, false},     {"dpd", 0, false},
	{"packet_analysis", 0, false}, {"file_analysis", 0, false}, {"tm", 0, false},
	{"logging", 0, false},         {"input", 0, false},         {"threading", 0, false},
	{"plugins", 0, false},         {"zeekygen", 0, false},      {"pktio", 0, false},
	{"broker", 0, false},          {"scripts", 0, false},       {"supervisor", 0, false},
	{"hashkey", 0, false},
};

DebugLogger::DebugLogger()
	{
	verbose = false;
	file = nullptr;
	}

DebugLogger::~DebugLogger()
	{
	if ( file && file != stderr )
		fclose(file);
	}

void DebugLogger::OpenDebugLog(const char* filename)
	{
	if ( filename )
		{
		filename = util::detail::log_file_name(filename);

		file = fopen(filename, "w");
		if ( ! file )
			{
			// The reporter may not be initialized here yet.
			if ( reporter )
				reporter->FatalError("can't open '%s' for debugging output", filename);
			else
				{
				fprintf(stderr, "can't open '%s' for debugging output\n", filename);
				exit(1);
				}
			}

		setvbuf(file, NULL, _IOLBF, 0);
		}
	else
		file = stderr;
	}

void DebugLogger::ShowStreamsHelp()
	{
	fprintf(stderr, "\n");
	fprintf(stderr, "Enable debug output into debug.log with -B <streams>.\n");
	fprintf(stderr, "<streams> is a comma-separated list of streams to enable.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Available streams:\n");

	for ( int i = 0; i < NUM_DBGS; ++i )
		fprintf(stderr, "  %s\n", streams[i].prefix);

	fprintf(stderr, "\n");
	fprintf(stderr, "  plugin-<plugin-name>   (replace '::' in name with '-'; e.g., '-B "
	                "plugin-Zeek-Netmap')\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Pseudo streams\n");
	fprintf(stderr, "  verbose  Increase verbosity.\n");
	fprintf(stderr, "  all      Enable all streams at maximum verbosity.\n");
	fprintf(stderr, "\n");
	}

void DebugLogger::EnableStreams(const char* s)
	{
	char* brkt;
	char* tmp = util::copy_string(s);
	char* tok = strtok(tmp, ",");

	while ( tok )
		{
		if ( strcasecmp("all", tok) == 0 )
			{
			for ( int i = 0; i < NUM_DBGS; ++i )
				{
				streams[i].enabled = true;
				enabled_streams.insert(streams[i].prefix);
				}

			verbose = true;
			goto next;
			}

		if ( strcasecmp("verbose", tok) == 0 )
			{
			verbose = true;
			goto next;
			}

		if ( strcasecmp("help", tok) == 0 )
			{
			ShowStreamsHelp();
			exit(0);
			}

		if ( util::starts_with(tok, "plugin-") )
			{
			// Cannot verify this at this time, plugins may not
			// have been loaded.
			enabled_streams.insert(tok);
			goto next;
			}

		int i;

		for ( i = 0; i < NUM_DBGS; ++i )
			{
			if ( strcasecmp(streams[i].prefix, tok) == 0 )
				{
				streams[i].enabled = true;
				enabled_streams.insert(tok);
				goto next;
				}
			}

		reporter->FatalError("unknown debug stream '%s', try -B help.\n", tok);

	next:
		tok = strtok(0, ",");
		}

	delete[] tmp;
	}

bool DebugLogger::CheckStreams(const std::set<std::string>& plugin_names)
	{
	bool ok = true;

	std::set<std::string> available_plugin_streams;
	for ( const auto& p : plugin_names )
		available_plugin_streams.insert(PluginStreamName(p));

	for ( auto const& stream : enabled_streams )
		{
		if ( ! util::starts_with(stream, "plugin-") )
			continue;

		if ( available_plugin_streams.count(stream) == 0 )
			{
			reporter->Error("No plugin debug stream '%s' found", stream.c_str());
			ok = false;
			}
		}

	return ok;
	}

void DebugLogger::Log(DebugStream stream, const char* fmt, ...)
	{
	Stream* g = &streams[int(stream)];

	if ( ! g->enabled )
		return;

	fprintf(file, "%17.06f/%17.06f [%s] ", run_state::network_time, util::current_time(true),
	        g->prefix);

	for ( int i = g->indent; i > 0; --i )
		fputs("   ", file);

	va_list ap;
	va_start(ap, fmt);
	vfprintf(file, fmt, ap);
	va_end(ap);

	fputc('\n', file);
	fflush(file);
	}

void DebugLogger::Log(const plugin::Plugin& plugin, const char* fmt, ...)
	{
	std::string tok = PluginStreamName(plugin.Name());

	if ( enabled_streams.find(tok) == enabled_streams.end() )
		return;

	fprintf(file, "%17.06f/%17.06f [plugin %s] ", run_state::network_time, util::current_time(true),
	        plugin.Name().c_str());

	va_list ap;
	va_start(ap, fmt);
	vfprintf(file, fmt, ap);
	va_end(ap);

	fputc('\n', file);
	fflush(file);
	}

	} // namespace zeek::detail

#endif
