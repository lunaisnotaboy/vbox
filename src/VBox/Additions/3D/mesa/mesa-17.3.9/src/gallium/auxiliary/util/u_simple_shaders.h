/**************************************************************************
 *
 * Copyright 2008 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef U_SIMPLE_SHADERS_H
#define U_SIMPLE_SHADERS_H


#include "pipe/p_compiler.h"
#include "pipe/p_shader_tokens.h"


struct pipe_context;
struct pipe_shader_state;
struct pipe_stream_output_info;


#ifdef __cplusplus
extern "C" {
#endif


extern void *
util_make_vertex_passthrough_shader(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes,
                                    bool window_space);

extern void *
util_make_vertex_passthrough_shader_with_so(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes,
                                    bool window_space, bool layered,
                                    const struct pipe_stream_output_info *so);

extern void *
util_make_layered_clear_vertex_shader(struct pipe_context *pipe);

extern void *
util_make_layered_clear_helper_vertex_shader(struct pipe_context *pipe);

extern void *
util_make_layered_clear_geometry_shader(struct pipe_context *pipe);

extern void *
util_make_fragment_tex_shader_writemask(struct pipe_context *pipe,
                                        unsigned tex_target,
                                        unsigned interp_mode,
                                        unsigned writemask,
                                        enum tgsi_return_type stype,
                                        enum tgsi_return_type dtype,
                                        bool load_level_zero,
                                        bool use_txf);

extern void *
util_make_fragment_tex_shader(struct pipe_context *pipe, unsigned tex_target,
                              unsigned interp_mode,
                              enum tgsi_return_type stype,
                              enum tgsi_return_type dtype,
                              bool load_level_zero,
                              bool use_txf);

extern void *
util_make_fragment_tex_shader_writedepth(struct pipe_context *pipe,
                                         unsigned tex_target,
                                         unsigned interp_mode,
                                         bool load_level_zero,
                                         bool use_txf);


extern void *
util_make_fragment_tex_shader_writedepthstencil(struct pipe_context *pipe,
                                                unsigned tex_target,
                                                unsigned interp_mode,
                                                bool load_level_zero,
                                                bool use_txf);


extern void *
util_make_fragment_tex_shader_writestencil(struct pipe_context *pipe,
                                           unsigned tex_target,
                                           unsigned interp_mode,
                                           bool load_level_zero,
                                           bool use_txf);


extern void *
util_make_fragment_passthrough_shader(struct pipe_context *pipe,
                                      int input_semantic,
                                      int input_interpolate,
                                      boolean write_all_cbufs);


extern void *
util_make_empty_fragment_shader(struct pipe_context *pipe);


extern void *
util_make_fragment_cloneinput_shader(struct pipe_context *pipe, int num_cbufs,
                                     int input_semantic,
                                     int input_interpolate);


extern void *
util_make_fs_blit_msaa_color(struct pipe_context *pipe,
                             enum tgsi_texture_type tgsi_tex,
                             enum tgsi_return_type stype,
                             enum tgsi_return_type dtype);


extern void *
util_make_fs_blit_msaa_depth(struct pipe_context *pipe,
                             enum tgsi_texture_type tgsi_tex);


extern void *
util_make_fs_blit_msaa_depthstencil(struct pipe_context *pipe,
                                    enum tgsi_texture_type tgsi_tex);


void *
util_make_fs_blit_msaa_stencil(struct pipe_context *pipe,
                               enum tgsi_texture_type tgsi_tex);


void *
util_make_fs_msaa_resolve(struct pipe_context *pipe,
                          enum tgsi_texture_type tgsi_tex, unsigned nr_samples,
                          enum tgsi_return_type stype);


void *
util_make_fs_msaa_resolve_bilinear(struct pipe_context *pipe,
                                   enum tgsi_texture_type tgsi_tex,
                                   unsigned nr_samples,
                                   enum tgsi_return_type stype);

extern void *
util_make_geometry_passthrough_shader(struct pipe_context *pipe,
                                      uint num_attribs,
                                      const ubyte *semantic_names,
                                      const ubyte *semantic_indexes);

#ifdef __cplusplus
}
#endif


#endif
