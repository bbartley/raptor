/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * raptor_serialize.c - Raptor Serializer API
 *
 * Copyright (C) 2004-2010, David Beckett http://www.dajobe.org/
 * Copyright (C) 2004-2005, University of Bristol, UK http://www.bristol.ac.uk/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 */


#ifdef HAVE_CONFIG_H
#include <raptor_config.h>
#endif

#ifdef WIN32
#include <win32_raptor_config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* Raptor includes */
#include "raptor.h"
#include "raptor_internal.h"


/* prototypes for helper functions */
static raptor_serializer_factory* raptor_get_serializer_factory(raptor_world* world, const char *name);


/* helper methods */

static void
raptor_free_serializer_factory(raptor_serializer_factory* factory)
{
  RAPTOR_ASSERT_OBJECT_POINTER_RETURN(factory, raptor_serializer_factory);

  if(factory->finish_factory)
    factory->finish_factory(factory);
  
  RAPTOR_FREE(raptor_serializer_factory, factory);
}


/* class methods */

int
raptor_serializers_init(raptor_world* world)
{
  int rc = 0;

  world->serializers = raptor_new_sequence((raptor_data_free_handler)raptor_free_serializer_factory, NULL);
  if(!world->serializers)
    return 1;

#ifdef RAPTOR_SERIALIZER_NTRIPLES
  rc += raptor_init_serializer_ntriples(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_TURTLE
  rc += raptor_init_serializer_turtle(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_RDFXML_ABBREV
  rc += raptor_init_serializer_rdfxmla(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_RDFXML
  rc += raptor_init_serializer_rdfxml(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_RSS_1_0
  rc += raptor_init_serializer_rss10(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_ATOM
  rc += raptor_init_serializer_atom(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_DOT
  rc += raptor_init_serializer_dot(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_JSON
  rc += raptor_init_serializer_json(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_HTML
  rc += raptor_init_serializer_html(world) != 0;
#endif

#ifdef RAPTOR_SERIALIZER_NQUADS
  rc += raptor_init_serializer_nquads(world) != 0;
#endif

  return rc;
}


/*
 * raptor_serializers_finish - delete all the registered serializers
 */
void
raptor_serializers_finish(raptor_world* world)
{
  if(world->serializers) {
    raptor_free_sequence(world->serializers);
    world->serializers = NULL;
  }
}


/*
 * raptor_serializer_register_factory:
 * @world: raptor_world object
 * @name: the short syntax name
 * @label: readable label for syntax
 * @mime_type: MIME type of the syntax generated by the serializer (or NULL)
 * @uri_string: URI string of the syntax (or NULL)
 * @factory: pointer to function to call to register the factory
 * 
 * INTERNAL - Register a syntax that can be generated by a serializer factory
 *
 * Return value: non-0 on failure
 **/
RAPTOR_EXTERN_C
raptor_serializer_factory*
raptor_serializer_register_factory(raptor_world* world,
                                   int (*factory) (raptor_serializer_factory*)) 
{
  raptor_serializer_factory *serializer;
  
  serializer = (raptor_serializer_factory*)RAPTOR_CALLOC(raptor_serializer_factory, 1,
                                                         sizeof(*serializer));
  if(!serializer)
    return NULL;

  serializer->world = world;

  serializer->desc.mime_types = NULL;
  
  if(raptor_sequence_push(world->serializers, serializer))
    return NULL; /* on error, serializer is already freed by the sequence */

  /* Call the serializer registration function on the new object */
  if(factory(serializer))
    return NULL; /* serializer is owned and freed by the serializers sequence */
  
  if(!serializer->desc.names || !serializer->desc.names[0] || 
     !serializer->desc.label) {
    raptor_log_error(world, RAPTOR_LOG_LEVEL_ERROR, NULL,
                     "Serializer failed to register required names and label fields\n");
    goto tidy;
  }

#ifdef RAPTOR_DEBUG
  /* Maintainer only check of static data */
  if(serializer->desc.mime_types) {
    unsigned int i;
    const raptor_type_q* type_q = NULL;

    for(i = 0; 
        (type_q = &serializer->desc.mime_types[i]) && type_q->mime_type;
        i++) {
      size_t len = strlen(type_q->mime_type);
      if(len != type_q->mime_type_len) {
        fprintf(stderr,
                "Serializer %s  mime type %s  actual len %d  static len %d\n",
                serializer->desc.names[0], type_q->mime_type,
                (int)len, (int)type_q->mime_type_len);
      }
    }

    if(i != serializer->desc.mime_types_count) {
        fprintf(stderr,
                "Serializer %s  saw %d mime types  static count %d\n",
                serializer->desc.names[0], i,
                serializer->desc.mime_types_count);
    }
  }
#endif

#if defined(RAPTOR_DEBUG) && RAPTOR_DEBUG > 1
  RAPTOR_DEBUG3("Registered serializer %s with context size %d\n",
                serializer->names[0], serializer->context_length);
#endif

  return serializer;

  /* Clean up on failure */
  tidy:
  raptor_free_serializer_factory(serializer);
  return NULL;
}


/**
 * raptor_get_serializer_factory:
 * @world: raptor_world object
 * @name: the factory name or NULL for the default factory
 *
 * Get a serializer factory by name.
 * 
 * Return value: the factory object or NULL if there is no such factory
 **/
static raptor_serializer_factory*
raptor_get_serializer_factory(raptor_world* world, const char *name) 
{
  raptor_serializer_factory *factory = NULL;

  RAPTOR_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, raptor_world, NULL);

  raptor_world_open(world);

  /* return 1st serializer if no particular one wanted - why? */
  if(!name) {
    factory = (raptor_serializer_factory *)raptor_sequence_get_at(world->serializers, 0);
    if(!factory) {
      RAPTOR_DEBUG1("No (default) serializers registered\n");
      return NULL;
    }
  } else {
    int i;
    
    for(i = 0;
        (factory = (raptor_serializer_factory*)raptor_sequence_get_at(world->serializers, i));
        i++) {
      int namei;
      const char* fname;
      
      for(namei = 0; (fname = factory->desc.names[namei]); namei++) {
        if(!strcmp(fname, name))
          break;
      }
      if(fname)
        break;
    }
  }
        
  return factory;
}


/**
 * raptor_world_get_serializer_description:
 * @world: world object
 * @counter: index into the list of serializers
 *
 * Get serializer descriptive syntax information
 * 
 * Return value: description or NULL if counter is out of range
 **/
const raptor_syntax_description*
raptor_world_get_serializer_description(raptor_world* world, 
                                        unsigned int counter)
{
  raptor_serializer_factory *factory;

  RAPTOR_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, raptor_world, NULL);

  raptor_world_open(world);

  factory = (raptor_serializer_factory*)raptor_sequence_get_at(world->serializers,
                                                               counter);

  if(!factory)
    return NULL;

  return &factory->desc;
}


/**
 * raptor_world_is_serializer_name:
 * @world: raptor_world object
 * @name: the syntax name
 *
 * Check name of a serializer.
 *
 * Return value: non 0 if name is a known syntax name
 */
int
raptor_world_is_serializer_name(raptor_world* world, const char *name)
{
  if(!name)
    return 0;
  
  RAPTOR_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, raptor_world, 0);

  raptor_world_open(world);

  return (raptor_get_serializer_factory(world, name) != NULL);
}


/**
 * raptor_new_serializer:
 * @world: raptor_world object
 * @name: the serializer name or NULL for default syntax
 *
 * Constructor - create a new raptor_serializer object.
 *
 * Return value: a new #raptor_serializer object or NULL on failure
 */
raptor_serializer*
raptor_new_serializer(raptor_world* world, const char *name)
{
  raptor_serializer_factory* factory;
  raptor_serializer* rdf_serializer;

  RAPTOR_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, raptor_world, NULL);

  raptor_world_open(world);

  factory = raptor_get_serializer_factory(world, name);
  if(!factory)
    return NULL;

  rdf_serializer = (raptor_serializer*)RAPTOR_CALLOC(raptor_serializer, 1,
                                                     sizeof(*rdf_serializer));
  if(!rdf_serializer)
    return NULL;

  rdf_serializer->world = world;
  
  rdf_serializer->context = (char*)RAPTOR_CALLOC(raptor_serializer_context, 1,
                                                 factory->context_length);
  if(!rdf_serializer->context) {
    raptor_free_serializer(rdf_serializer);
    return NULL;
  }
  
  rdf_serializer->factory = factory;

  raptor_object_options_init(&rdf_serializer->options,
                             RAPTOR_OPTION_AREA_SERIALIZER);

  /* Default options (that are not 0 or NULL) */
  /* Emit @base directive or equivalent */
  RAPTOR_OPTIONS_SET_NUMERIC(rdf_serializer, RAPTOR_OPTION_WRITE_BASE_URI, 1);
  
  /* Emit relative URIs where possible */
  RAPTOR_OPTIONS_SET_NUMERIC(rdf_serializer, RAPTOR_OPTION_RELATIVE_URIS, 1);

  /* XML 1.0 output */
  RAPTOR_OPTIONS_SET_NUMERIC(rdf_serializer,
                             RAPTOR_OPTION_WRITER_XML_VERSION, 10);

  /* Write XML declaration */
  RAPTOR_OPTIONS_SET_NUMERIC(rdf_serializer,
                             RAPTOR_OPTION_WRITER_XML_DECLARATION, 1);

  if(factory->init(rdf_serializer, name)) {
    raptor_free_serializer(rdf_serializer);
    return NULL;
  }
  
  return rdf_serializer;
}


/**
 * raptor_serializer_start_to_iostream:
 * @rdf_serializer:  the #raptor_serializer
 * @uri: base URI or NULL if no base URI is required
 * @iostream: #raptor_iostream to write serialization to
 * 
 * Start serialization to an iostream with given base URI
 *
 * The passed in @iostream does not become owned by the serializer
 * and can be used by the caller after serializing is done.  It
 * must be destroyed by the caller.
 *
 * Return value: non-0 on failure.
 **/
int
raptor_serializer_start_to_iostream(raptor_serializer *rdf_serializer,
                                    raptor_uri *uri, raptor_iostream *iostream) 
{
  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  if(!iostream)
    return 1;
  
  if(uri)
    uri = raptor_uri_copy(uri);
  
  rdf_serializer->base_uri = uri;
  rdf_serializer->locator.uri = uri;
  rdf_serializer->locator.line = rdf_serializer->locator.column = 0;

  rdf_serializer->iostream = iostream;

  rdf_serializer->free_iostream_on_end = 0;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}


/**
 * raptor_serializer_start_to_filename:
 * @rdf_serializer:  the #raptor_serializer
 * @filename:  filename to serialize to
 *
 * Start serializing to a filename.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serializer_start_to_filename(raptor_serializer *rdf_serializer,
                                    const char *filename)
{
  unsigned char *uri_string = raptor_uri_filename_to_uri_string(filename);
  if(!uri_string)
    return 1;

  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  rdf_serializer->base_uri = raptor_new_uri(rdf_serializer->world, uri_string);
  rdf_serializer->locator.uri = rdf_serializer->base_uri;
  rdf_serializer->locator.line = rdf_serializer->locator.column = 0;

  RAPTOR_FREE(cstring, uri_string);

  rdf_serializer->iostream = raptor_new_iostream_to_filename(rdf_serializer->world,
                                                             filename);
  if(!rdf_serializer->iostream)
    return 1;

  rdf_serializer->free_iostream_on_end = 1;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}



/**
 * raptor_serializer_start_to_string:
 * @rdf_serializer:  the #raptor_serializer
 * @uri: base URI or NULL if no base URI is required
 * @string_p: pointer to location to hold string
 * @length_p: pointer to location to hold length of string (or NULL)
 *
 * Start serializing to a string.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serializer_start_to_string(raptor_serializer *rdf_serializer,
                                  raptor_uri *uri,
                                  void **string_p, size_t *length_p) 
{
  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  if(uri)
    rdf_serializer->base_uri = raptor_uri_copy(uri);
  else
    rdf_serializer->base_uri = NULL;
  rdf_serializer->locator.uri = rdf_serializer->base_uri;
  rdf_serializer->locator.line = rdf_serializer->locator.column = 0;


  rdf_serializer->iostream = raptor_new_iostream_to_string(rdf_serializer->world,
                                                           string_p, length_p, 
                                                           NULL);
  if(!rdf_serializer->iostream)
    return 1;

  rdf_serializer->free_iostream_on_end = 1;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}


/**
 * raptor_serializer_start_to_file_handle:
 * @rdf_serializer:  the #raptor_serializer
 * @uri: base URI or NULL if no base URI is required
 * @fh:  FILE* to serialize to
 *
 * Start serializing to a FILE*.
 * 
 * NOTE: This does not fclose the handle when it is finished.
 *
 * Return value: non-0 on failure.
 **/
int
raptor_serializer_start_to_file_handle(raptor_serializer *rdf_serializer,
                                       raptor_uri *uri, FILE *fh) 
{
  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  if(uri)
    rdf_serializer->base_uri = raptor_uri_copy(uri);
  else
    rdf_serializer->base_uri = NULL;
  rdf_serializer->locator.uri = rdf_serializer->base_uri;
  rdf_serializer->locator.line = rdf_serializer->locator.column = 0;

  rdf_serializer->iostream = raptor_new_iostream_to_file_handle(rdf_serializer->world, fh);
  if(!rdf_serializer->iostream)
    return 1;

  rdf_serializer->free_iostream_on_end = 1;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}


/**
 * raptor_serializer_set_namespace:
 * @rdf_serializer: the #raptor_serializer
 * @uri: #raptor_uri of namespace or NULL
 * @prefix: prefix to use or NULL
 *
 * set a namespace uri/prefix mapping for serializing.
 *
 * return value: non-0 on failure.
 **/
int
raptor_serializer_set_namespace(raptor_serializer* rdf_serializer,
                                raptor_uri *uri, const unsigned char *prefix) 
{
  if(prefix && !*prefix)
    prefix = NULL;
  
  if(rdf_serializer->factory->declare_namespace)
    return rdf_serializer->factory->declare_namespace(rdf_serializer, 
                                                      uri, prefix);

  return 1;
}


/**
 * raptor_serializer_set_namespace_from_namespace:
 * @rdf_serializer: the #raptor_serializer
 * @nspace: #raptor_namespace to set
 *
 * Set a namespace uri/prefix mapping for serializing from an existing namespace.
 *
 * Return value: non-0 on failure.
 **/
int
raptor_serializer_set_namespace_from_namespace(raptor_serializer* rdf_serializer,
                                               raptor_namespace *nspace)
{
  if(rdf_serializer->factory->declare_namespace_from_namespace)
    return rdf_serializer->factory->declare_namespace_from_namespace(rdf_serializer, 
                                                                     nspace);
  else if(rdf_serializer->factory->declare_namespace)
    return rdf_serializer->factory->declare_namespace(rdf_serializer, 
                                                      raptor_namespace_get_uri(nspace),
                                                      raptor_namespace_get_prefix(nspace));

  return 1;
}


/**
 * raptor_serializer_serialize_statement:
 * @rdf_serializer: the #raptor_serializer
 * @statement: #raptor_statement to serialize to a syntax
 *
 * Serialize a statement.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serializer_serialize_statement(raptor_serializer* rdf_serializer,
                                      raptor_statement *statement)
{
  if(!rdf_serializer->iostream)
    return 1;

  return rdf_serializer->factory->serialize_statement(rdf_serializer,
                                                      statement);
}


/**
 * raptor_serializer_serialize_end:
 * @rdf_serializer:  the #raptor_serializer
 *
 * End a serialization.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serializer_serialize_end(raptor_serializer *rdf_serializer) 
{
  int rc;
  
  if(!rdf_serializer->iostream)
    return 1;

  if(rdf_serializer->factory->serialize_end)
    rc = rdf_serializer->factory->serialize_end(rdf_serializer);
  else
    rc = 0;

  if(rdf_serializer->iostream) {
    if(rdf_serializer->free_iostream_on_end)
      raptor_free_iostream(rdf_serializer->iostream);
    rdf_serializer->iostream = NULL;
  }
  return rc;
}



/**
 * raptor_free_serializer:
 * @rdf_serializer: #raptor_serializer object
 *
 * Destructor - destroy a raptor_serializer object.
 * 
 **/
void
raptor_free_serializer(raptor_serializer* rdf_serializer) 
{
  if(!rdf_serializer)
    return;

  if(rdf_serializer->factory)
    rdf_serializer->factory->terminate(rdf_serializer);

  if(rdf_serializer->context)
    RAPTOR_FREE(raptor_serializer_context, rdf_serializer->context);

  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  raptor_object_options_clear(&rdf_serializer->options);

  RAPTOR_FREE(raptor_serializer, rdf_serializer);
}


/**
 * raptor_serializer_get_iostream:
 * @serializer: #raptor_serializer object
 *
 * Get the current serializer iostream.
 *
 * Return value: the serializer's current iostream or NULL if 
 **/
raptor_iostream*
raptor_serializer_get_iostream(raptor_serializer *serializer)
{
  return serializer->iostream;
}


/**
 * raptor_serializer_set_option:
 * @serializer: #raptor_serializer serializer object
 * @option: option to set from enumerated #raptor_option values
 * @string: string option value (or NULL)
 * @integer: integer option value
 *
 * Set serializer option.
 * 
 * If @string is not NULL and the option type is numeric, the string
 * value is converted to an integer and used in preference to @integer.
 *
 * If @string is NULL and the option type is not numeric, an error is
 * returned.
 *
 * The @string values used are copied.
 *
 * The allowed options are available via
 * raptor_world_get_option_description().
 *
 * Return value: non 0 on failure or if the option is unknown
 **/
int
raptor_serializer_set_option(raptor_serializer *serializer, 
                             raptor_option option, 
                             const char* string, int integer)
{
  return raptor_object_options_set_option(&serializer->options, option,
                                          string, integer);
}


/**
 * raptor_serializer_get_option:
 * @serializer: #raptor_serializer serializer object
 * @option: option to get value
 * @string_p: pointer to where to store string value
 * @integer_p: pointer to where to store integer value
 *
 * Get serializer option.
 * 
 * Any string value returned in *@string_p is shared and must
 * be copied by the caller.
 *
 * The allowed options are available via
 * raptor_world_get_option_description().
 *
 * Return value: option value or < 0 for an illegal option
 **/
int
raptor_serializer_get_option(raptor_serializer *serializer, 
                             raptor_option option,
                             char** string_p, int* integer_p)
{
  return raptor_object_options_get_option(&serializer->options, option,
                                          string_p, integer_p);
}


/**
 * raptor_serializer_get_locator:
 * @rdf_serializer: raptor serializer
 *
 * Get the serializer raptor locator object.
 * 
 * Return value: raptor locator
 **/
raptor_locator*
raptor_serializer_get_locator(raptor_serializer *rdf_serializer)
{
  return &rdf_serializer->locator;
}


/**
 * raptor_serializer_get_world:
 * @rdf_serializer: raptor serializer
 * 
 * Get the #raptor_world object associated with a serializer.
 *
 * Return value: raptor_world* pointer
 **/
raptor_world *
raptor_serializer_get_world(raptor_serializer* rdf_serializer)
{
  return rdf_serializer->world;
}


/**
 * raptor_serializer_get_description:
 * @rdf_serializer: #raptor_serializer serializer object
 *
 * Get description of the syntaxes of the serializer.
 *
 * The returned description is static and lives as long as the raptor
 * library (raptor world).
 *
 * Return value: description of syntax
 **/
const raptor_syntax_description*
raptor_serializer_get_description(raptor_serializer *rdf_serializer)
{
  return &rdf_serializer->factory->desc;
}


/**
 * raptor_serializer_flush:
 * @rdf_serializer: raptor serializer
 *
 * Flush the current serializer output and free any pending state
 *
 * In serializers that can generate blocks of content, this causes
 * the writing of any current pending block.  For example in Turtle
 * this may write all pending triples.
 * 
 * Return value: non-0 on failure
 **/
int
raptor_serializer_flush(raptor_serializer *rdf_serializer)
{
  int rc;
  
  if(rdf_serializer->factory->serialize_flush)
    rc = rdf_serializer->factory->serialize_flush(rdf_serializer);
  else
    rc = 0;

  return rc;
}
