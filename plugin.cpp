/*
 * FogLAMP "scale" filter plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Massimiliano Pinto
 */

#include <plugin_api.h>
#include <config_category.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <iostream>
#include <filter_plugin.h>
#include <filter.h>
#include <reading_set.h>

#define FILTER_NAME "scale"
#define SCALE_FACTOR "100.0"
#define DEFAULT_CONFIG "{\"plugin\" : { \"description\" : \"Scale filter plugin\", " \
                       		"\"type\" : \"string\", " \
				"\"default\" : \"" FILTER_NAME "\" }, " \
			 "\"enable\": {\"description\": \"A switch that can be used to enable or disable execution of " \
					 "the scale filter.\", " \
				"\"type\": \"boolean\", " \
				"\"default\": \"false\" }, " \
			"\"factor\" : {\"description\" : \"Scale factor for a reading value.\", " \
				"\"type\": \"float\", " \
				"\"default\": \"" SCALE_FACTOR "\"} }"
using namespace std;

/**
 * The Filter plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
        FILTER_NAME,              // Name
        "1.0.0",                  // Version
        0,                        // Flags
        PLUGIN_TYPE_FILTER,       // Type
        "1.0.0",                  // Interface version
	DEFAULT_CONFIG	          // Default plugin configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Initialise the plugin, called to get the plugin handle and setup the
 * output handle that will be passed to the output stream. The output stream
 * is merely a function pointer that is called with the output handle and
 * the new set of readings generated by the plugin.
 *     (*output)(outHandle, readings);
 * Note that the plugin may not call the output stream if the result of
 * the filtering is that no readings are to be sent onwards in the chain.
 * This allows the plugin to discard data or to buffer it for aggregation
 * with data that follows in subsequent calls
 *
 * @param config	The configuration category for the filter
 * @param outHandle	A handle that will be passed to the output stream
 * @param output	The output stream (function pointer) to which data is passed
 * @return		An opaque handle that is used in all subsequent calls to the plugin
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* config,
			  OUTPUT_HANDLE *outHandle,
			  OUTPUT_STREAM output)
{
	FogLampFilter* handle = new FogLampFilter(FILTER_NAME,
						  *config,
						  outHandle,
						  output);

	return (PLUGIN_HANDLE)handle;
}

/**
 * Ingest a set of readings into the plugin for processing
 *
 * @param handle	The plugin handle returned from plugin_init
 * @param readingSet	The readings to process
 */
void plugin_ingest(PLUGIN_HANDLE *handle,
		   READINGSET *readingSet)
{
	FogLampFilter* filter = (FogLampFilter *)handle;
	if (!filter->isEnabled())
	{
		// Current filter is not active: just pass the readings set
		filter->m_func(filter->m_data, readingSet);
		return;
	}

	// Get filter configuration
	double scaleFactor;
	if (filter->getConfig().itemExists("factor"))
	{
		scaleFactor = strtod(filter->getConfig().getValue("factor").c_str(), NULL);
	}
	else
	{
		scaleFactor = strtod(SCALE_FACTOR, NULL);
	}

	// 1- We might need to transform the inout readings set: example
	// ReadingSet* newReadings = scale_readings(scaleFactor, readingSet);

	// Just get all the readings in the readingset
	const vector<Reading *>& readings = ((ReadingSet *)readingSet)->getAllReadings();
	// Iterate the readings
	for (vector<Reading *>::const_iterator elem = readings.begin();
						      elem != readings.end();
						      ++elem)
	{
		// Get a reading DataPoint
		const vector<Datapoint *>& dataPoints = (*elem)->getReadingData();
		// Iterate the datapoints
		for (vector<Datapoint *>::const_iterator it = dataPoints.begin(); it != dataPoints.end(); ++it)
		{
			// Get the reference to a DataPointValue
			DatapointValue& value = (*it)->getData();

			// If INTEGER or FLOAT do the change
			if (value.getType() == DatapointValue::T_INTEGER)
			{
				value.setValue((long)(value.toInt() * scaleFactor));
			}
			else if (value.getType() == DatapointValue::T_FLOAT)
			{
				value.setValue(value.toDouble() * scaleFactor);
			}
			else
			{
				// do nothing
			}
		}
	}

	// 2- optionally free reading set
	// delete (ReadingSet *)readingSet;
	// With the above DataPointValue change we don't need to free input data

	// 3- pass newReadings to filter->m_func instead of readings if needed.
	// With the value change we can pass same input readingset just modified
	filter->m_func(filter->m_data, readingSet);
}

/**
 * Call the shutdown method in the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE *handle)
{
	FogLampFilter* data = (FogLampFilter *)handle;
	delete data;
}

// End of extern "C"
};
