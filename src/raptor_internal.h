/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * raptor_internal.h - Redland Parser Toolkit for RDF (Raptor) internals
 *
 * $Id$
 *
 * Copyright (C) 2002 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
 * University of Bristol - http://www.bristol.ac.uk/
 * 
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL)
 *   2. GNU General Public License (GPL)
 *   3. Mozilla Public License (MPL)
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * full license terms.
 * 
 */



#ifndef RAPTOR_INTERNAL_H
#define RAPTOR_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RAPTOR_INTERNAL

#ifdef RAPTOR_IN_REDLAND
#define LIBRDF_INTERNAL
#endif
  
#ifdef LIBRDF_INTERNAL
#ifdef LIBRDF_DEBUG
#define RAPTOR_DEBUG 1
#endif

#define IS_RDF_MS_CONCEPT(name, uri, local_name) librdf_uri_equals(uri, librdf_concept_uris[LIBRDF_CONCEPT_MS_##local_name])
#define RAPTOR_URI_AS_STRING(uri) (librdf_uri_as_string(uri))
#define RAPTOR_URI_TO_FILENAME(uri) (librdf_uri_to_filename(uri))
#define RAPTOR_FREE_URI(uri) librdf_free_uri(uri)
#else
/* else standalone */

#define LIBRDF_MALLOC(type, size) malloc(size)
#define LIBRDF_CALLOC(type, size, count) calloc(size, count)
#define LIBRDF_FREE(type, ptr)   free((void*)ptr)

#define IS_RDF_MS_CONCEPT(name, uri, local_name) !strcmp(name, #local_name)
#define RAPTOR_URI_AS_STRING(uri) ((const char*)uri)
#define RAPTOR_URI_TO_FILENAME(uri) (raptor_uri_uri_string_to_filename(uri))
#define RAPTOR_FREE_URI(uri) LIBRDF_FREE(cstring, uri)

#ifdef RAPTOR_DEBUG
/* Debugging messages */
#define LIBRDF_DEBUG1(function, msg) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function); } while(0)
#define LIBRDF_DEBUG2(function, msg, arg1) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1);} while(0)
#define LIBRDF_DEBUG3(function, msg, arg1, arg2) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2);} while(0)
#define LIBRDF_DEBUG4(function, msg, arg1, arg2, arg3) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2, arg3);} while(0)
#define LIBRDF_DEBUG4(function, msg, arg1, arg2, arg3) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2, arg3);} while(0)

#else
/* DEBUGGING TURNED OFF */

/* No debugging messages */
#define LIBRDF_DEBUG1(function, msg)
#define LIBRDF_DEBUG2(function, msg, arg1)
#define LIBRDF_DEBUG3(function, msg, arg1, arg2)
#define LIBRDF_DEBUG4(function, msg, arg1, arg2, arg3)

#endif


/* Fatal errors - always happen */
#define LIBRDF_FATAL1(function, msg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg, __FILE__, __LINE__ , #function); abort();} while(0)
#define LIBRDF_FATAL2(function, msg,arg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg, __FILE__, __LINE__ , #function, arg); abort();} while(0)

#endif



/* XML parser includes */
#ifdef RAPTOR_XML_EXPAT
#ifdef HAVE_EXPAT_H
#include <expat.h>
#endif
#ifdef HAVE_XMLPARSE_H
#include <xmlparse.h>
#endif
#endif


#ifdef RAPTOR_XML_LIBXML

#ifdef HAVE_LIBXML_PARSER_H
#include <libxml/parser.h>
#else
#ifdef HAVE_GNOME_XML_PARSER_H
#include <gnome-xml/parser.h>
#else
DIE
#endif
#endif

#define XML_Char xmlChar


/*
 * Raptor entity expansion list
 * (libxml only)
 */
#ifdef RAPTOR_LIBXML_MY_ENTITIES

struct raptor_xml_entity_t {
  xmlEntity entity;
#ifndef RAPTOR_LIBXML_ENTITY_NAME_LENGTH
  int name_length;
#endif

  struct raptor_xml_entity_t *next;
};
typedef struct raptor_xml_entity_t raptor_xml_entity;
#ifdef RAPTOR_LIBXML_ENTITY_NAME_LENGTH
#define RAPTOR_ENTITY_NAME_LENGTH(ent) ent->entity.name_length
#else
#define RAPTOR_ENTITY_NAME_LENGTH(ent) ent->name_length
#endif

#endif


/* libxml-only prototypes */


/* raptor_libxml.c exports */
extern void raptor_libxml_init(xmlSAXHandler *sax);
#ifdef RAPTOR_LIBXML_MY_ENTITIES
extern void raptor_libxml_free_entities(raptor_parser *rdf_parser);
#endif
extern void raptor_libxml_validation_error(void *context, const char *msg, ...);
extern void raptor_libxml_validation_warning(void *context, const char *msg, ...);

/* raptor_parse.c - exported to libxml part */
extern xmlParserCtxtPtr raptor_get_libxml_context(raptor_parser *rdf_parser);
extern void raptor_set_libxml_document_locator(raptor_parser *rdf_parser, xmlSAXLocatorPtr loc);
extern xmlSAXLocatorPtr raptor_get_libxml_document_locator(raptor_parser *rdf_parser);
extern void raptor_libxml_update_document_locator (raptor_parser *rdf_parser);

#ifdef RAPTOR_LIBXML_MY_ENTITIES
extern raptor_xml_entity* raptor_get_libxml_entities(raptor_parser *rdf_parser);
extern void raptor_set_libxml_entities(raptor_parser *rdf_parser, raptor_xml_entity* entities);
#endif
/* end of libxml-only */
#endif


extern void raptor_parser_fatal_error(raptor_parser* parser, const char *message, ...);
extern void raptor_parser_error(raptor_parser* parser, const char *message, ...);
extern void raptor_parser_warning(raptor_parser* parser, const char *message, ...);
extern void raptor_parser_fatal_error_varargs(raptor_parser* parser, const char *message, va_list arguments);
extern void raptor_parser_error_varargs(raptor_parser* parser, const char *message, va_list arguments);
extern void raptor_parser_warning_varargs(raptor_parser* parser, const char *message, va_list arguments);


/* raptor_parse.c */

/* Prototypes for common expat/libxml parsing event-handling functions */
extern void raptor_xml_start_element_handler(void *user_data, const XML_Char *name, const XML_Char **atts);
extern void raptor_xml_end_element_handler(void *user_data, const XML_Char *name);
/* s is not 0 terminated. */
extern void raptor_xml_cdata_handler(void *user_data, const XML_Char *s, int len);

extern void raptor_expat_update_document_locator (raptor_parser *rdf_parser);


/* raptor_locator.c */
extern void raptor_update_document_locator (raptor_parser *rdf_parser);


#ifdef HAVE_STRCASECMP
#define raptor_strcasecmp strcasecmp
#define raptor_strncasecmp strncasecmp
#else
#ifdef HAVE_STRICMP
#define raptor_strcasecmp stricmp
#define raptor_strncasecmp strnicmp
#endif
#endif


/* end of RAPTOR_INTERNAL */
#endif


#ifdef __cplusplus
}
#endif

#endif
