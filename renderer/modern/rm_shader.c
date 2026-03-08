/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// rm_shader.c - Modern renderer GLSL shader loader/compiler implementation
//

#include "../r_local.h"
#include "rm_shader.h"
#include "../glad/glad.h"
#include <string.h>
#include <stdio.h>

/* Shader list head */
static rmShader_t *rm_shaderList = NULL;

/* Include tracking for cycle detection */
typedef struct {
	char files[RM_SHADER_MAX_INCLUDES][64];
	int count;
} includeStack_t;

/*
==================
RM_CheckIncludeCycle

Checks if a file is already in the include stack (cycle detection).
Returns: qTrue if cycle detected, qFalse otherwise
==================
*/
static qBool RM_CheckIncludeCycle (includeStack_t *stack, const char *filename)
{
	int i;
	for (i = 0; i < stack->count; i++) {
		if (!strcmp (stack->files[i], filename)) {
			return qTrue;
		}
	}
	return qFalse;
}

/*
==================
RM_ProcessIncludes

Processes #include directives in shader source.
Returns: Processed source string (must be freed), or NULL on error
==================
*/
static char *RM_ProcessIncludes (const char *source, const char *baseDir, includeStack_t *stack, int depth)
{
	char *result;
	char *line;
	char *src;
	char includePath[256];
	char includeFile[128];
	char *includeSource;
	char *processedInclude;
	int resultLen;
	int lineLen;
	
	if (depth >= 16) {
		Com_Printf (PRNT_ERROR, "Shader #include depth limit exceeded (max 16)\n");
		return NULL;
	}
	
	/* Allocate initial result buffer */
	resultLen = strlen (source) * 2 + 1024; /* generous */
	result = (char *)malloc (resultLen);
	if (!result) {
		Com_Printf (PRNT_ERROR, "Shader: Failed to allocate memory for include processing\n");
		return NULL;
	}
	result[0] = '\0';
	
	/* Process line by line */
	src = (char *)source;
	while (*src) {
		/* Find end of line */
		line = src;
		while (*src && *src != '\n') src++;
		lineLen = (int)(src - line);
		if (*src == '\n') src++;
		
		/* Check for #include directive */
		if (lineLen > 10 && !strncmp (line, "#include", 8)) {
			/* Extract filename */
			char *quote1 = strchr (line, '"');
			char *quote2 = quote1 ? strchr (quote1 + 1, '"') : NULL;
			
			if (quote1 && quote2) {
				int nameLen = (int)(quote2 - quote1 - 1);
				if (nameLen > 0 && nameLen < sizeof(includeFile) - 1) {
					strncpy (includeFile, quote1 + 1, nameLen);
					includeFile[nameLen] = '\0';
					
					/* Check for cycle */
					if (RM_CheckIncludeCycle (stack, includeFile)) {
						Com_Printf (PRNT_ERROR, "Shader: Include cycle detected: %s\n", includeFile);
						free (result);
						return NULL;
					}
					
					/* Build full path */
					Q_snprintfz (includePath, sizeof(includePath), "%s/%s", baseDir, includeFile);
					
					/* Load include file */
					if (FS_LoadFile (includePath, (void **)&includeSource, "\0") <= 0) {
						Com_Printf (PRNT_ERROR, "Shader: Failed to load include file: %s\n", includePath);
						free (result);
						return NULL;
					}
					
					/* Push to stack */
					if (stack->count < RM_SHADER_MAX_INCLUDES) {
						Q_strncpyz (stack->files[stack->count], includeFile, sizeof(stack->files[0]));
						stack->count++;
					}
					
					/* Recursively process includes */
					processedInclude = RM_ProcessIncludes (includeSource, baseDir, stack, depth + 1);
					/* includeSource is allocated by FS_LoadFile, will be freed when processed */
					
					if (!processedInclude) {
						free (result);
						return NULL;
					}
					
					/* Append processed include */
					strcat (result, processedInclude);
					strcat (result, "\n");
					free (processedInclude);
					
					/* Pop from stack */
					if (stack->count > 0) stack->count--;
					
					continue;
				}
			}
			
			Com_Printf (PRNT_WARNING, "Shader: Malformed #include directive\n");
		}
		
		/* Append line to result */
		strncat (result, line, lineLen);
		strcat (result, "\n");
	}
	
	return result;
}

/*
==================
RM_InjectVersion

Injects #version directive if not present.
Returns: Source with version (must be freed), or NULL on error
==================
*/
static char *RM_InjectVersion (const char *source)
{
	char *result;
	int resultLen;
	
	/* Check if #version already present */
	if (strstr (source, "#version")) {
		/* Already has version, return copy */
		result = (char *)malloc (strlen (source) + 1);
		if (result) {
			strcpy (result, source);
		}
		return result;
	}
	
	/* Inject default version */
	resultLen = strlen (RM_SHADER_DEFAULT_VERSION) + strlen (source) + 1;
	result = (char *)malloc (resultLen);
	if (!result) {
		return NULL;
	}
	
	strcpy (result, RM_SHADER_DEFAULT_VERSION);
	strcat (result, source);
	
	return result;
}

/*
==================
RM_CompileShader

Compiles a shader stage.
Returns: Shader object ID, or 0 on failure
==================
*/
static GLuint RM_CompileShader (GLenum type, const char *source, const char *name)
{
	GLuint shader;
	GLint compiled;
	GLint logLength;
	char *log;
	const char *typeStr = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
	FILE *f;
	
	/* Debug: dump shader source to file */
	{
		char dumpPath[128];
		Q_snprintfz(dumpPath, sizeof(dumpPath), "%s_%s_dump.glsl", name, typeStr);
		f = fopen(dumpPath, "w");
		if (f) {
			fprintf(f, "%s", source);
			fclose(f);
		}
	}
	
	shader = glCreateShader (type);
	if (!shader) {
		Com_Printf (PRNT_ERROR, "Shader %s: Failed to create %s shader object\n", name, typeStr);
		return 0;
	}
	
	glShaderSource (shader, 1, &source, NULL);
	glCompileShader (shader);
	
	glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		Com_Printf (PRNT_ERROR, "Shader %s: %s compilation FAILED\n", name, typeStr);
		
		/* Log to file for debugging */
		f = fopen("modern.log", "a");
		if (f) {
			fprintf(f, "SHADER COMPILE ERROR: %s (%s)\n", name, typeStr);
		}
		
		glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 1) {
			log = (char *)malloc (logLength);
			if (log) {
				glGetShaderInfoLog (shader, logLength, NULL, log);
				Com_Printf (PRNT_ERROR, "--- Compilation Log ---\n%s\n", log);
				if (f) {
					fprintf(f, "ERROR LOG:\n%s\n", log);
				}
				free (log);
			}
		}
		
		if (f) fclose(f);
		glDeleteShader (shader);
		return 0;
	}
	
	Com_Printf (0, "Shader %s: %s compiled successfully\n", name, typeStr);
	return shader;
}

/*
==================
RM_LinkProgram

Links vertex and fragment shaders into a program.
Returns: qTrue on success, qFalse on failure
==================
*/
static qBool RM_LinkProgram (GLuint program, const char *name)
{
	GLint linked;
	GLint logLength;
	char *log;
	
	glLinkProgram (program);
	
	glGetProgramiv (program, GL_LINK_STATUS, &linked);
	if (!linked) {
		Com_Printf (PRNT_ERROR, "Shader %s: Program linking FAILED\n", name);
		
		glGetProgramiv (program, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 1) {
			log = (char *)malloc (logLength);
			if (log) {
				glGetProgramInfoLog (program, logLength, NULL, log);
				Com_Printf (PRNT_ERROR, "--- Linking Log ---\n%s\n", log);
				free (log);
			}
		}
		
		return qFalse;
	}
	
	Com_Printf (0, "Shader %s: Program linked successfully\n", name);
	return qTrue;
}

/*
==================
RM_LoadShaderFile

Loads a shader file with #include processing and #version injection.
Returns: Processed source (must be freed), or NULL on error
==================
*/
static char *RM_LoadShaderFile (const char *path)
{
	char *rawSource;
	char *processedSource;
	char *finalSource;
	char *nullTerminated;
	includeStack_t stack;
	char baseDir[256];
	char *lastSlash;
	
	/* Load raw source - use NULL terminate to get raw data */
	int fileSize = FS_LoadFile ((char *)path, (void **)&rawSource, NULL);
	if (fileSize <= 0) {
		Com_Printf (PRNT_ERROR, "  Failed to load: %s (fileSize=%d)\n", path, fileSize);
		return NULL;
	}
	
	/* Explicitly null-terminate the shader source */
	nullTerminated = (char *)malloc(fileSize + 1);
	if (!nullTerminated) {
		FS_FreeFile(rawSource);
		return NULL;
	}
	memcpy(nullTerminated, rawSource, fileSize);
	nullTerminated[fileSize] = '\0';
	FS_FreeFile(rawSource);
	rawSource = nullTerminated;
	
	Com_Printf (0, "  Loaded: %s (%d bytes)\n", path, fileSize);
	
	/* Extract base directory for includes */
	Q_strncpyz (baseDir, path, sizeof(baseDir));
	lastSlash = strrchr (baseDir, '/');
	if (lastSlash) {
		*lastSlash = '\0';
	} else {
		baseDir[0] = '\0';
	}
	
	/* Process includes */
	memset (&stack, 0, sizeof(stack));
	processedSource = RM_ProcessIncludes (rawSource, baseDir, &stack, 0);
	free (rawSource); /* Free our malloc'd null-terminated copy */
	
	if (!processedSource) {
		return NULL;
	}
	
	/* Inject version if needed */
	finalSource = RM_InjectVersion (processedSource);
	free (processedSource);
	
	return finalSource;
}

/*
==================
RM_InitShaderSystem

Initializes the modern shader subsystem.
==================
*/
void RM_InitShaderSystem (void)
{
	Com_Printf (0, "Initializing modern shader system...\n");
	rm_shaderList = NULL;
	Com_Printf (0, "Modern shader system initialized.\n");
}

/*
==================
RM_ShutdownShaderSystem

Shuts down the shader subsystem.
==================
*/
void RM_ShutdownShaderSystem (void)
{
	rmShader_t *shader, *next;
	
	Com_Printf (0, "Shutting down modern shader system...\n");
	
	shader = rm_shaderList;
	while (shader) {
		next = shader->next;
		
		if (shader->program) {
			glDeleteProgram (shader->program);
		}
		if (shader->vertShader) {
			glDeleteShader (shader->vertShader);
		}
		if (shader->fragShader) {
			glDeleteShader (shader->fragShader);
		}
		
		free (shader);
		shader = next;
	}
	
	rm_shaderList = NULL;
	Com_Printf (0, "Modern shader system shutdown complete.\n");
}

/*
==================
RM_FindShader

Finds a loaded shader by name.
==================
*/
rmShader_t *RM_FindShader (const char *name)
{
	rmShader_t *shader;
	
	for (shader = rm_shaderList; shader; shader = shader->next) {
		if (!strcmp (shader->name, name)) {
			return shader;
		}
	}
	
	return NULL;
}

/*
==================
RM_LoadShader

Loads, compiles, and links a shader program.
==================
*/
rmShader_t *RM_LoadShader (const char *name)
{
	rmShader_t *shader;
	char vertPath[256];
	char fragPath[256];
	char *vertSource;
	char *fragSource;
	GLuint program;
	
	/* Check if already loaded */
	shader = RM_FindShader (name);
	if (shader) {
		Com_Printf (PRNT_WARNING, "Shader %s already loaded\n", name);
		return shader;
	}
	
	Com_Printf (0, "Loading shader: %s\n", name);
	
	/* Build virtual paths (use engine filesystem) */
	Q_snprintfz (vertPath, sizeof(vertPath), "shaders/%s.vert", name);
	Q_snprintfz (fragPath, sizeof(fragPath), "shaders/%s.frag", name);
	
	Com_Printf (0, "  Vertex shader: %s\n", vertPath);
	Com_Printf (0, "  Fragment shader: %s\n", fragPath);
	
	/* Load and process vertex shader */
	vertSource = RM_LoadShaderFile (vertPath);
	if (!vertSource) {
		Com_Printf (PRNT_ERROR, "Shader %s: Failed to load vertex shader: %s\n", name, vertPath);
		return NULL;
	}
	
	/* Load and process fragment shader */
	fragSource = RM_LoadShaderFile (fragPath);
	if (!fragSource) {
		Com_Printf (PRNT_ERROR, "Shader %s: Failed to load fragment shader: %s\n", name, fragPath);
		free (vertSource);
		return NULL;
	}
	
	/* Allocate shader structure */
	shader = (rmShader_t *)malloc (sizeof(rmShader_t));
	if (!shader) {
		Com_Printf (PRNT_ERROR, "Shader %s: Failed to allocate shader structure\n", name);
		free (vertSource);
		free (fragSource);
		return NULL;
	}
	
	memset (shader, 0, sizeof(rmShader_t));
	Q_strncpyz (shader->name, name, sizeof(shader->name));
	
	/* Compile shaders */
	shader->vertShader = RM_CompileShader (GL_VERTEX_SHADER, vertSource, name);
	free (vertSource);
	
	if (!shader->vertShader) {
		free (fragSource);
		free (shader);
		return NULL;
	}
	
	shader->fragShader = RM_CompileShader (GL_FRAGMENT_SHADER, fragSource, name);
	free (fragSource);
	
	if (!shader->fragShader) {
		glDeleteShader (shader->vertShader);
		free (shader);
		return NULL;
	}
	
	/* Create and link program */
	program = glCreateProgram ();
	if (!program) {
		Com_Printf (PRNT_ERROR, "Shader %s: Failed to create program object\n", name);
		glDeleteShader (shader->vertShader);
		glDeleteShader (shader->fragShader);
		free (shader);
		return NULL;
	}
	
	glAttachShader (program, shader->vertShader);
	glAttachShader (program, shader->fragShader);
	
	if (!RM_LinkProgram (program, name)) {
		glDeleteProgram (program);
		glDeleteShader (shader->vertShader);
		glDeleteShader (shader->fragShader);
		free (shader);
		return NULL;
	}
	
	shader->program = program;
	shader->valid = qTrue;
	
	/* Add to list */
	shader->next = rm_shaderList;
	rm_shaderList = shader;
	
	Com_Printf (0, "Shader %s: PASS (program ID %u)\n", name, program);
	return shader;
}

/*
==================
RM_ReloadShader

Reloads a shader from disk.
==================
*/
qBool RM_ReloadShader (const char *name)
{
	rmShader_t *shader;
	rmShader_t *prev;
	
	/* Find and remove from list */
	prev = NULL;
	for (shader = rm_shaderList; shader; shader = shader->next) {
		if (!strcmp (shader->name, name)) {
			if (prev) {
				prev->next = shader->next;
			} else {
				rm_shaderList = shader->next;
			}
			break;
		}
		prev = shader;
	}
	
	if (!shader) {
		Com_Printf (PRNT_WARNING, "Shader %s not loaded, cannot reload\n", name);
		return qFalse;
	}
	
	/* Delete old resources */
	if (shader->program) {
		glDeleteProgram (shader->program);
	}
	if (shader->vertShader) {
		glDeleteShader (shader->vertShader);
	}
	if (shader->fragShader) {
		glDeleteShader (shader->fragShader);
	}
	free (shader);
	
	/* Reload */
	Com_Printf (0, "Reloading shader: %s\n", name);
	shader = RM_LoadShader (name);
	
	return (shader != NULL);
}

/*
==================
RM_ReloadAllShaders

Reloads all loaded shaders.
==================
*/
void RM_ReloadAllShaders (void)
{
	rmShader_t *shader;
	char names[256][64];
	int count = 0;
	int i;
	
	/* Collect names */
	for (shader = rm_shaderList; shader && count < 256; shader = shader->next) {
		Q_strncpyz (names[count], shader->name, sizeof(names[0]));
		count++;
	}
	
	Com_Printf (0, "Reloading %d shader(s)...\n", count);
	
	/* Reload each */
	for (i = 0; i < count; i++) {
		RM_ReloadShader (names[i]);
	}
}

/*
==================
RM_ListShaders

Lists all loaded shaders.
==================
*/
void RM_ListShaders (void)
{
	rmShader_t *shader;
	int count = 0;
	
	Com_Printf (0, "Loaded shaders:\n");
	for (shader = rm_shaderList; shader; shader = shader->next) {
		Com_Printf (0, "  %s (program %u) %s\n", 
			shader->name, 
			shader->program,
			shader->valid ? "VALID" : "INVALID");
		count++;
	}
	Com_Printf (0, "Total: %d shader(s)\n", count);
}

/*
==================
Console Commands
==================
*/

void RM_ShaderTest_f (void)
{
	rmShader_t *shader;
	
	if (Cmd_Argc () < 2) {
		Com_Printf (0, "Usage: shader_test <name>\n");
		Com_Printf (0, "Loads shaders/<name>.vert + shaders/<name>.frag\n");
		return;
	}
	
	shader = RM_LoadShader (Cmd_Argv (1));
	if (shader) {
		Com_Printf (0, "shader_test: PASS\n");
	} else {
		Com_Printf (PRNT_ERROR, "shader_test: FAIL\n");
	}
}

void RM_ShaderList_f (void)
{
	RM_ListShaders ();
}

void RM_ShaderReload_f (void)
{
	if (Cmd_Argc () < 2) {
		Com_Printf (0, "Usage: shader_reload <name|all>\n");
		return;
	}
	
	if (!strcmp (Cmd_Argv (1), "all")) {
		RM_ReloadAllShaders ();
	} else {
		if (RM_ReloadShader (Cmd_Argv (1))) {
			Com_Printf (0, "shader_reload: SUCCESS\n");
		} else {
			Com_Printf (PRNT_ERROR, "shader_reload: FAILED\n");
		}
	}
}
