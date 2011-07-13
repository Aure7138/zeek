##! This is the core Bro script support for the notion of a cluster manager.
##!
##! The manager is passive (the workers connect to us), and once connected
##! the manager registers for the events on the workers that are needed
##! to get the desired data from the workers.

##! This is where the cluster manager sets it's specific settings for other
##! frameworks and in the core.

## Turn off remote logging since this is the manager and should only log here.
redef Log::enable_remote_logging = F;

## Use the cluster's archive logging script.
redef Log::default_rotation_postprocessor = "archive-log";

## The cluster manager does not capture packets.
redef interfaces = "";

## We're processing essentially *only* remote events.
redef max_remote_events_processed = 10000;

# Reraise remote notices locally.
event Notice::notice(n: Notice::Info)
	{
	if ( is_remote_event() )
		#if ( FilterDuplicates::is_new(n) )
		NOTICE(n);
	}