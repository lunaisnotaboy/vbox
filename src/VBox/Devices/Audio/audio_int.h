/*
 * QEMU Audio subsystem header
 *
 * Copyright (c) 2003-2005 Vassili Karpov (malc)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef QEMU_AUDIO_INT_H
#define QEMU_AUDIO_INT_H

#ifdef CONFIG_COREAUDIO
#ifndef VBOX
#define FLOAT_MIXENG
#else
#undef  FLOAT_MIXENG
#endif
/* #define RECIPROCAL */
#endif

#include <limits.h>
#include "mixeng.h"

#define qemu_malloc RTMemAlloc
#define qemu_mallocz RTMemAllocZ
#define qemu_free RTMemFree
#define qemu_strdup RTStrDup
#define qemu_strfree RTStrFree
#define asprintf     RTStrAPrintf

struct audio_pcm_ops;

typedef enum {
    AUD_OPT_INT,
    AUD_OPT_FMT,
    AUD_OPT_STR,
    AUD_OPT_BOOL
} audio_option_tag_e;

struct audio_option {
    const char *name;
    audio_option_tag_e tag;
    void *valp;
    const char *descr;
    int *overridenp;
    int overriden;
};

struct audio_callback {
    void *opaque;
    audio_callback_fn_t fn;
};

struct audio_pcm_info {
    int bits;
    int sign;
    int freq;
    int nchannels;
    int align;
    int shift;
    int bytes_per_second;
    int swap_endianness;
};

typedef struct SWVoiceCap SWVoiceCap;

typedef struct HWVoiceOut {
    int enabled;
    int pending_disable;
    struct audio_pcm_info info;

    f_sample *clip;

    int rpos;
    uint64_t ts_helper;

    st_sample_t *mix_buf;

    int samples;
    LIST_HEAD (sw_out_listhead, SWVoiceOut) sw_head;
    LIST_HEAD (sw_cap_listhead, SWVoiceCap) cap_head;
    struct audio_pcm_ops *pcm_ops;
    LIST_ENTRY (HWVoiceOut) entries;
} HWVoiceOut;

typedef struct HWVoiceIn {
    int enabled;
    struct audio_pcm_info info;

    t_sample *conv;

    int wpos;
    int total_samples_captured;
    uint64_t ts_helper;

    st_sample_t *conv_buf;

    int samples;
    LIST_HEAD (sw_in_listhead, SWVoiceIn) sw_head;
    struct audio_pcm_ops *pcm_ops;
    LIST_ENTRY (HWVoiceIn) entries;
} HWVoiceIn;

struct SWVoiceOut {
    struct audio_pcm_info info;
    t_sample *conv;
    int64_t ratio;
    st_sample_t *buf;
    void *rate;
    int total_hw_samples_mixed;
    int active;
    int empty;
    HWVoiceOut *hw;
    char *name;
    volume_t vol;
    struct audio_callback callback;
    LIST_ENTRY (SWVoiceOut) entries;
};

struct SWVoiceIn {
    int active;
    struct audio_pcm_info info;
    int64_t ratio;
    void *rate;
    int total_hw_samples_acquired;
    st_sample_t *buf;
    f_sample *clip;
    HWVoiceIn *hw;
    char *name;
    volume_t vol;
    struct audio_callback callback;
    LIST_ENTRY (SWVoiceIn) entries;
};

struct audio_driver {
    const char *name;
    const char *descr;
    struct audio_option *options;
    void *(*init) (void);
    void (*fini) (void *);
    struct audio_pcm_ops *pcm_ops;
    int can_be_default;
    int max_voices_out;
    int max_voices_in;
    int voice_size_out;
    int voice_size_in;
};

struct audio_pcm_ops {
    int  (*init_out)(HWVoiceOut *hw, audsettings_t *as);
    void (*fini_out)(HWVoiceOut *hw);
    int  (*run_out) (HWVoiceOut *hw);
    int  (*write)   (SWVoiceOut *sw, void *buf, int size);
    int  (*ctl_out) (HWVoiceOut *hw, int cmd, ...);

    int  (*init_in) (HWVoiceIn *hw, audsettings_t *as);
    void (*fini_in) (HWVoiceIn *hw);
    int  (*run_in)  (HWVoiceIn *hw);
    int  (*read)    (SWVoiceIn *sw, void *buf, int size);
    int  (*ctl_in)  (HWVoiceIn *hw, int cmd, ...);
};

struct capture_callback {
    struct audio_capture_ops ops;
    void *opaque;
    LIST_ENTRY (capture_callback) entries;
};

struct CaptureVoiceOut {
    HWVoiceOut hw;
    void *buf;
    LIST_HEAD (cb_listhead, capture_callback) cb_head;
    LIST_ENTRY (CaptureVoiceOut) entries;
};

struct SWVoiceCap {
    SWVoiceOut sw;
    CaptureVoiceOut *cap;
    LIST_ENTRY (SWVoiceCap) entries;
};

struct AudioState {
    struct audio_driver *drv;
    void *drv_opaque;

    QEMUTimer *ts;
    LIST_HEAD (card_listhead, QEMUSoundCard) card_head;
    LIST_HEAD (hw_in_listhead, HWVoiceIn) hw_head_in;
    LIST_HEAD (hw_out_listhead, HWVoiceOut) hw_head_out;
    LIST_HEAD (cap_listhead, CaptureVoiceOut) cap_head;
    int nb_hw_voices_out;
    int nb_hw_voices_in;
    PPDMDRVINS pDrvIns;
};

extern struct audio_driver no_audio_driver;
extern struct audio_driver oss_audio_driver;
extern struct audio_driver sdl_audio_driver;
extern struct audio_driver wav_audio_driver;
extern struct audio_driver fmod_audio_driver;
extern struct audio_driver alsa_audio_driver;
extern struct audio_driver pulse_audio_driver;
extern struct audio_driver coreaudio_audio_driver;
extern struct audio_driver dsound_audio_driver;
extern struct audio_driver solaudio_audio_driver;
extern volume_t nominal_volume;
#ifdef VBOX
extern volume_t pcm_out_volume;
extern volume_t pcm_in_volume;
#endif

uint64_t audio_get_clock (void);
uint64_t audio_get_ticks_per_sec (void);

void audio_pcm_init_info (struct audio_pcm_info *info, audsettings_t *as);
void audio_pcm_info_clear_buf (struct audio_pcm_info *info, void *buf, int len);

int  audio_pcm_sw_write (SWVoiceOut *sw, void *buf, int len);
int  audio_pcm_hw_get_live_in (HWVoiceIn *hw);

int  audio_pcm_sw_read (SWVoiceIn *sw, void *buf, int len);
int  audio_pcm_hw_get_live_out (HWVoiceOut *hw);
int  audio_pcm_hw_get_live_out2 (HWVoiceOut *hw, int *nb_live);

int audio_bug (const char *funcname, int cond);
void *audio_calloc (const char *funcname, int nmemb, size_t size);

#define VOICE_ENABLE 1
#define VOICE_DISABLE 2

static inline int audio_ring_dist (int dst, int src, int len)
{
    return (dst >= src) ? (dst - src) : (len - src + dst);
}

#if defined __GNUC__ && !defined VBOX /* VBox: oh, please, just shut up. */
#define GCC_ATTR __attribute__ ((__unused__, __format__ (__printf__, 1, 2)))
#if __STDC_VERSION__ > 199901L
#define INIT_FIELD(f) . f
#else
#define INIT_FIELD(f) /**/
#endif
#define GCC_FMT_ATTR(n, m) __attribute__ ((__format__ (__printf__, n, m)))
#else
#define GCC_ATTR /**/
#define INIT_FIELD(f) /**/
#define GCC_FMT_ATTR(n, m)
#endif

#ifndef VBOX
static void GCC_ATTR dolog (const char *fmt, ...)
#else
DECLINLINE(void) GCC_ATTR dolog (const char *fmt, ...) /* shuts up unused warnings. */
#endif
{
    va_list ap;

    va_start (ap, fmt);
    AUD_vlog (AUDIO_CAP, fmt, ap);
    va_end (ap);
}

#ifdef DEBUG
DECLINLINE(void) GCC_ATTR ldebug (const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    AUD_vlog (AUDIO_CAP, fmt, ap);
    va_end (ap);
}
#else
#if defined NDEBUG && defined __GNUC__
#define ldebug(...)
#elif defined NDEBUG && defined _MSC_VER
#define ldebug __noop
#else
#ifndef VBOX
static void GCC_ATTR ldebug (const char *fmt, ...)
#else
DECLINLINE(void) GCC_ATTR ldebug (const char *fmt, ...)  /* shuts up unused warnings. */
#endif
{
    (void) fmt;
}
#endif
#endif

#undef GCC_ATTR

#define AUDIO_STRINGIFY_(n) #n
#define AUDIO_STRINGIFY(n) AUDIO_STRINGIFY_(n)

#if defined _MSC_VER || defined __GNUC__
#define AUDIO_FUNC __FUNCTION__
#else
#define AUDIO_FUNC __FILE__ ":" AUDIO_STRINGIFY (__LINE__)
#endif

DECLCALLBACK(bool) sniffer_run_out (HWVoiceOut *hw, void *pvSamples,
                                    unsigned cSamples);

/*
 * Filter interface.
 */
typedef DECLCALLBACK(int) FNAUDIOINPUTCALLBACK(void* pvCtx, uint32_t cbSamples, const void *pvSamples);
typedef FNAUDIOINPUTCALLBACK *PFNAUDIOINPUTCALLBACK;

int filter_output_intercepted(void);
int filter_output_begin(void **ppvOutputCtx, struct audio_pcm_info *pinfo, int samples);
void filter_output_end(void *pvOutputCtx);

int filter_input_intercepted(void);
int filter_input_begin(void **ppvInputCtx, PFNAUDIOINPUTCALLBACK pfnCallback, void *pvCallback, HWVoiceIn *phw, int samples);
void filter_input_end(void *pvInputCtx);

struct audio_driver *filteraudio_install(struct audio_driver *pDrv, void *pDrvOpaque);

#endif /* audio_int.h */
