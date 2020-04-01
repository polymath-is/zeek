// See the file "COPYING" in the main distribution directory for copyright.

#include "Configuration.h"
#include "utils.h"

#include "Reporter.h"

#include <fstream>
#include <vector>
#include <algorithm>
#include <errno.h>

using namespace zeekygen;
using namespace std;

static TargetFactory create_target_factory()
	{
	TargetFactory rval;
	rval.Register<PackageIndexTarget>("package_index");
	rval.Register<PackageTarget>("package");
	rval.Register<ProtoAnalyzerTarget>("proto_analyzer");
	rval.Register<FileAnalyzerTarget>("file_analyzer");
	rval.Register<ScriptSummaryTarget>("script_summary");
	rval.Register<ScriptIndexTarget>("script_index");
	rval.Register<ScriptTarget>("script");
	rval.Register<IdentifierTarget>("identifier");
	return rval;
	}

Config::Config(const string& arg_file, const string& delim)
	: file(arg_file), targets(), target_factory(create_target_factory())
	{
	if ( file.empty() )
		return;

	ifstream f(file.c_str());

	if ( ! f.is_open() )
		reporter->FatalError("failed to open Zeekygen config file '{:s}': {:s}",
		                     file, strerror(errno));

	string line;
	unsigned int line_number = 0;

	while ( getline(f, line) )
		{
		++line_number;
		vector<string> tokens;
		tokenize_string(line, delim, &tokens);
		tokens.erase(remove(tokens.begin(), tokens.end(), ""), tokens.end());

		if ( tokens.empty() )
			// Blank line.
			continue;

		if ( ! tokens[0].empty() && tokens[0][0] == '#' )
			// Comment
			continue;

		if ( tokens.size() != 3 )
			reporter->FatalError("malformed Zeekygen target in {:s}:{:d}: {:s}",
			                     file, line_number, line);

		Target* target = target_factory.Create(tokens[0], tokens[2], tokens[1]);

		if ( ! target )
			reporter->FatalError("unknown Zeekygen target type: {:s}",
			                     tokens[0]);

		targets.push_back(target);
		}

	if ( f.bad() )
		reporter->InternalError("error reading Zeekygen config file '{:s}': {:s}",
		                        file, strerror(errno));
	}

Config::~Config()
	{
	for ( size_t i = 0; i < targets.size(); ++i )
		delete targets[i];
	}

void Config::FindDependencies(const vector<Info*>& infos)
	{
	for ( size_t i = 0; i < targets.size(); ++i )
		targets[i]->FindDependencies(infos);
	}

void Config::GenerateDocs() const
	{
	for ( size_t i = 0; i < targets.size(); ++i )
		targets[i]->Generate();
	}

time_t Config::GetModificationTime() const
	{
	if ( file.empty() )
		return 0;

	return zeekygen::get_mtime(file);
	}
