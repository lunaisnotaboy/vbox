/*
 * mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file vbo_context.h
 * \brief VBO builder module datatypes and definitions.
 * \author Keith Whitwell
 */


/**
 * \mainpage The VBO builder module
 *
 * This module hooks into the GL dispatch table and catches all vertex
 * building and drawing commands, such as glVertex3f, glBegin and
 * glDrawArrays.  The module stores all incoming vertex data as arrays
 * in GL vertex buffer objects (VBOs), and translates all drawing
 * commands into calls to a driver supplied DrawPrimitives() callback.
 *
 * The module captures both immediate mode and display list drawing,
 * and manages the allocation, reference counting and deallocation of
 * vertex buffer objects itself.
 * 
 * The DrawPrimitives() callback can be either implemented by the
 * driver itself or hooked to the tnl module's _tnl_draw_primitives()
 * function for hardware without tnl capablilties or during fallbacks.
 */


#ifndef _VBO_CONTEXT_H
#define _VBO_CONTEXT_H

#include "vbo.h"
#include "vbo_attrib.h"
#include "vbo_exec.h"
#include "vbo_save.h"

#include "main/api_arrayelt.h"
#include "main/macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vbo_context {
   struct gl_vertex_array currval[VBO_ATTRIB_MAX];
   
   /** Map VERT_ATTRIB_x to VBO_ATTRIB_y */
   GLuint map_vp_none[VERT_ATTRIB_MAX];
   GLuint map_vp_arb[VERT_ATTRIB_MAX];

   struct vbo_exec_context exec;
   struct vbo_save_context save;

   /* Callback into the driver.  This must always succeed, the driver
    * is responsible for initiating any fallback actions required:
    */
   vbo_draw_func draw_prims;

   /* Optional callback for indirect draws. This allows multidraws to not be
    * broken up, as well as for the actual count to be passed in as a separate
    * indirect parameter.
    */
   vbo_indirect_draw_func draw_indirect_prims;
};


static inline struct vbo_context *vbo_context(struct gl_context *ctx) 
{
   return ctx->vbo_context;
}


static inline void
vbo_exec_invalidate_state(struct gl_context *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;

   if (ctx->NewState & (_NEW_PROGRAM | _NEW_ARRAY)) {
      if (!exec->validating)
         exec->array.recalculate_inputs = GL_TRUE;

      _ae_invalidate_state(ctx);
   }

   if (ctx->NewState & _NEW_EVAL)
      exec->eval.recalculate_maps = GL_TRUE;
}


/**
 * Return VP_x token to indicate whether we're running fixed-function
 * vertex transformation, an NV vertex program or ARB vertex program/shader.
 */
static inline enum vp_mode
get_program_mode( struct gl_context *ctx )
{
   if (!ctx->VertexProgram._Current)
      return VP_NONE;
   else if (ctx->VertexProgram._Current == ctx->VertexProgram._TnlProgram)
      return VP_NONE;
   else
      return VP_ARB;
}


/**
 * This is called by glBegin, glDrawArrays and glDrawElements (and
 * variations of those calls).  When we transition from immediate mode
 * drawing to array drawing we need to invalidate the array state.
 *
 * glBegin/End builds vertex arrays.  Those arrays may look identical
 * to glDrawArrays arrays except that the position of the elements may
 * be different.  For example, arrays of (position3v, normal3f) vs. arrays
 * of (normal3f, position3f).  So we need to make sure we notify drivers
 * that arrays may be changing.
 */
static inline void
vbo_draw_method(struct vbo_context *vbo, gl_draw_method method)
{
   struct gl_context *ctx = vbo->exec.ctx;

   if (ctx->Array.DrawMethod != method) {
      switch (method) {
      case DRAW_ARRAYS:
         ctx->Array._DrawArrays = vbo->exec.array.inputs;
         break;
      case DRAW_BEGIN_END:
         ctx->Array._DrawArrays = vbo->exec.vtx.inputs;
         break;
      case DRAW_DISPLAY_LIST:
         ctx->Array._DrawArrays = vbo->save.inputs;
         break;
      default:
         assert(0);
      }

      ctx->NewDriverState |= ctx->DriverFlags.NewArray;
      ctx->Array.DrawMethod = method;
   }
}

/**
 * Return if format is integer. The immediate mode commands only emit floats
 * for non-integer types, thus everything else is integer.
 */
static inline GLboolean
vbo_attrtype_to_integer_flag(GLenum format)
{
   switch (format) {
   case GL_FLOAT:
   case GL_DOUBLE:
      return GL_FALSE;
   case GL_INT:
   case GL_UNSIGNED_INT:
   case GL_UNSIGNED_INT64_ARB:
      return GL_TRUE;
   default:
      assert(0);
      return GL_FALSE;
   }
}

static inline GLboolean
vbo_attrtype_to_double_flag(GLenum format)
{
   switch (format) {
   case GL_FLOAT:
   case GL_INT:
   case GL_UNSIGNED_INT:
   case GL_UNSIGNED_INT64_ARB:
      return GL_FALSE;
   case GL_DOUBLE:
      return GL_TRUE;
   default:
      assert(0);
      return GL_FALSE;
   }
}

/**
 * Return default component values for the given format.
 * The return type is an array of fi_types, because that's how we declare
 * the vertex storage : floats , integers or unsigned integers.
 */
static inline const fi_type *
vbo_get_default_vals_as_union(GLenum format)
{
   static const GLfloat default_float[4] = { 0, 0, 0, 1 };
   static const GLint default_int[4] = { 0, 0, 0, 1 };

   switch (format) {
   case GL_FLOAT:
      return (fi_type *)default_float;
   case GL_INT:
   case GL_UNSIGNED_INT:
      return (fi_type *)default_int;
   default:
      assert(0);
      return NULL;
   }
}


/**
 * Compute the max number of vertices which can be stored in
 * a vertex buffer, given the current vertex size, and the amount
 * of space already used.
 */
static inline unsigned
vbo_compute_max_verts(const struct vbo_exec_context *exec)
{
   unsigned n = (VBO_VERT_BUFFER_SIZE - exec->vtx.buffer_used) /
      (exec->vtx.vertex_size * sizeof(GLfloat));
   if (n == 0)
      return 0;
   /* Subtract one so we're always sure to have room for an extra
    * vertex for GL_LINE_LOOP -> GL_LINE_STRIP conversion.
    */
   n--;
   return n;
}


#ifdef __cplusplus
} // extern "C"
#endif

#endif
