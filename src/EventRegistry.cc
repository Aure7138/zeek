#include "zeek/EventRegistry.h"

#include "zeek/EventHandler.h"
#include "zeek/Func.h"
#include "zeek/RE.h"
#include "zeek/Reporter.h"

namespace zeek
	{

EventRegistry::EventRegistry() = default;
EventRegistry::~EventRegistry() noexcept = default;

EventHandlerPtr EventRegistry::Register(std::string_view name, bool is_from_script)
	{
	// If there already is an entry in the registry, we have a
	// local handler on the script layer.
	EventHandler* h = event_registry->Lookup(name);

	if ( h )
		{
		if ( ! is_from_script )
			not_only_from_script.insert(std::string(name));

		h->SetUsed();
		return h;
		}

	h = new EventHandler(std::string(name));
	event_registry->Register(h, is_from_script);

	h->SetUsed();

	return h;
	}

void EventRegistry::Register(EventHandlerPtr handler, bool is_from_script)
	{
	std::string name = handler->Name();

	handlers[name] = std::unique_ptr<EventHandler>(handler.Ptr());

	if ( ! is_from_script )
		not_only_from_script.insert(name);
	}

EventHandler* EventRegistry::Lookup(std::string_view name)
	{
	auto it = handlers.find(name);
	if ( it != handlers.end() )
		return it->second.get();

	return nullptr;
	}

bool EventRegistry::NotOnlyRegisteredFromScript(std::string_view name)
	{
	return not_only_from_script.count(std::string(name)) > 0;
	}

EventRegistry::string_list EventRegistry::Match(RE_Matcher* pattern)
	{
	string_list names;

	for ( const auto& entry : handlers )
		{
		EventHandler* v = entry.second.get();
		if ( v->GetFunc() && pattern->MatchExactly(v->Name()) )
			names.push_back(entry.first);
		}

	return names;
	}

EventRegistry::string_list EventRegistry::UnusedHandlers()
	{
	string_list names;

	for ( const auto& entry : handlers )
		{
		EventHandler* v = entry.second.get();
		if ( v->GetFunc() && ! v->Used() )
			names.push_back(entry.first);
		}

	return names;
	}

EventRegistry::string_list EventRegistry::UsedHandlers()
	{
	string_list names;

	for ( const auto& entry : handlers )
		{
		EventHandler* v = entry.second.get();
		if ( v->GetFunc() && v->Used() )
			names.push_back(entry.first);
		}

	return names;
	}

EventRegistry::string_list EventRegistry::AllHandlers()
	{
	string_list names;

	for ( const auto& entry : handlers )
		{
		names.push_back(entry.first);
		}

	return names;
	}

void EventRegistry::PrintDebug()
	{
	for ( const auto& entry : handlers )
		{
		EventHandler* v = entry.second.get();
		fprintf(stderr, "Registered event %s (%s handler / %s)\n", v->Name(),
		        v->GetFunc() ? "local" : "no", *v ? "active" : "not active");
		}
	}

void EventRegistry::SetErrorHandler(std::string_view name)
	{
	EventHandler* eh = Lookup(name);

	if ( eh )
		{
		eh->SetErrorHandler();
		return;
		}

	reporter->InternalWarning("unknown event handler '%s' in SetErrorHandler()",
	                          std::string(name).c_str());
	}

	} // namespace zeek
