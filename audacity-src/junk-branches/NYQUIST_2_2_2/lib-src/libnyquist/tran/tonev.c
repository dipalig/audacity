#include "stdio.h"
#ifndef mips
#include "stdlib.h"
#endif
#include "xlisp.h"
#include "sound.h"

#include "falloc.h"
#include "cext.h"
#include "tonev.h"

void tonev_free();


typedef struct tonev_susp_struct {
    snd_susp_node susp;
    boolean started;
    long terminate_cnt;
    boolean logically_stopped;
    sound_type s1;
    long s1_cnt;
    sample_block_values_type s1_ptr;
    sound_type hz;
    long hz_cnt;
    sample_block_values_type hz_ptr;

    /* support for interpolation of hz */
    sample_type hz_x1_sample;
    double hz_pHaSe;
    double hz_pHaSe_iNcR;

    /* support for ramp between samples of hz */
    double output_per_hz;
    long hz_n;

    double scale1;
    double c2;
    double c1;
    double prev;
} tonev_susp_node, *tonev_susp_type;


void tonev_ns_fetch(register tonev_susp_type susp, snd_list_type snd_list)
{
    int cnt = 0; /* how many samples computed */
    int togo;
    int n;
    sample_block_type out;
    register sample_block_values_type out_ptr;

    register sample_block_values_type out_ptr_reg;

    register double scale1_reg;
    register double c2_reg;
    register double c1_reg;
    register double prev_reg;
    register sample_type hz_scale_reg = susp->hz->scale;
    register sample_block_values_type hz_ptr_reg;
    register sample_block_values_type s1_ptr_reg;
    falloc_sample_block(out, "tonev_ns_fetch");
    out_ptr = out->samples;
    snd_list->block = out;

    while (cnt < max_sample_block_len) { /* outer loop */
    /* first compute how many samples to generate in inner loop: */
    /* don't overflow the output sample block: */
    togo = max_sample_block_len - cnt;

    /* don't run past the s1 input sample block: */
    susp_check_term_log_samples(s1, s1_ptr, s1_cnt);
    togo = min(togo, susp->s1_cnt);

    /* don't run past the hz input sample block: */
    susp_check_term_samples(hz, hz_ptr, hz_cnt);
    togo = min(togo, susp->hz_cnt);

    /* don't run past terminate time */
    if (susp->terminate_cnt != UNKNOWN &&
        susp->terminate_cnt <= susp->susp.current + cnt + togo) {
        togo = susp->terminate_cnt - (susp->susp.current + cnt);
        if (togo == 0) break;
    }


    /* don't run past logical stop time */
    if (!susp->logically_stopped && susp->susp.log_stop_cnt != UNKNOWN) {
        int to_stop = susp->susp.log_stop_cnt - (susp->susp.current + cnt);
        /* break if to_stop == 0 (we're at the logical stop)
         * AND cnt > 0 (we're not at the beginning of the
         * output block).
         */
        if (to_stop < togo) {
        if (to_stop == 0) {
            if (cnt) {
            togo = 0;
            break;
            } else /* keep togo as is: since cnt == 0, we
                    * can set the logical stop flag on this
                    * output block
                    */
            susp->logically_stopped = true;
        } else /* limit togo so we can start a new
                * block at the LST
                */
            togo = to_stop;
        }
    }

    n = togo;
    scale1_reg = susp->scale1;
    c2_reg = susp->c2;
    c1_reg = susp->c1;
    prev_reg = susp->prev;
    hz_ptr_reg = susp->hz_ptr;
    s1_ptr_reg = susp->s1_ptr;
    out_ptr_reg = out_ptr;
    if (n) do { /* the inner sample computation loop */
        register double b;
        b = 2.0 - cos((hz_scale_reg * *hz_ptr_reg++));
        c2_reg = b - sqrt((b * b) - 1.0);
        c1_reg = (1.0 - c2_reg) * scale1_reg;
            *out_ptr_reg++ = (sample_type) (prev_reg = c1_reg * *s1_ptr_reg++ + c2_reg * prev_reg);
    } while (--n); /* inner loop */

    susp->prev = prev_reg;
    /* using hz_ptr_reg is a bad idea on RS/6000: */
    susp->hz_ptr += togo;
    /* using s1_ptr_reg is a bad idea on RS/6000: */
    susp->s1_ptr += togo;
    out_ptr += togo;
    susp_took(s1_cnt, togo);
    susp_took(hz_cnt, togo);
    cnt += togo;
    } /* outer loop */

    /* test for termination */
    if (togo == 0 && cnt == 0) {
    snd_list_terminate(snd_list);
    } else {
    snd_list->block_len = cnt;
    susp->susp.current += cnt;
    }
    /* test for logical stop */
    if (susp->logically_stopped) {
    snd_list->logically_stopped = true;
    } else if (susp->susp.log_stop_cnt == susp->susp.current) {
    susp->logically_stopped = true;
    }
} /* tonev_ns_fetch */


void tonev_ni_fetch(register tonev_susp_type susp, snd_list_type snd_list)
{
    int cnt = 0; /* how many samples computed */
    int togo;
    int n;
    sample_block_type out;
    register sample_block_values_type out_ptr;

    register sample_block_values_type out_ptr_reg;

    register double scale1_reg;
    register double c2_reg;
    register double c1_reg;
    register double prev_reg;
    register double hz_pHaSe_iNcR_rEg = susp->hz_pHaSe_iNcR;
    register double hz_pHaSe_ReG;
    register sample_type hz_x1_sample_reg;
    register sample_block_values_type s1_ptr_reg;
    falloc_sample_block(out, "tonev_ni_fetch");
    out_ptr = out->samples;
    snd_list->block = out;

    /* make sure sounds are primed with first values */
    if (!susp->started) {
        register double b;
    susp->started = true;
    susp_check_term_samples(hz, hz_ptr, hz_cnt);
    susp->hz_x1_sample = susp_fetch_sample(hz, hz_ptr, hz_cnt);
    b = 2.0 - cos(susp->hz_x1_sample);
    susp->c2 = b - sqrt((b * b) - 1.0);
    susp->c1 = (1.0 - susp->c2) * susp->scale1;
    }

    while (cnt < max_sample_block_len) { /* outer loop */
    /* first compute how many samples to generate in inner loop: */
    /* don't overflow the output sample block: */
    togo = max_sample_block_len - cnt;

    /* don't run past the s1 input sample block: */
    susp_check_term_log_samples(s1, s1_ptr, s1_cnt);
    togo = min(togo, susp->s1_cnt);

    /* don't run past terminate time */
    if (susp->terminate_cnt != UNKNOWN &&
        susp->terminate_cnt <= susp->susp.current + cnt + togo) {
        togo = susp->terminate_cnt - (susp->susp.current + cnt);
        if (togo == 0) break;
    }


    /* don't run past logical stop time */
    if (!susp->logically_stopped && susp->susp.log_stop_cnt != UNKNOWN) {
        int to_stop = susp->susp.log_stop_cnt - (susp->susp.current + cnt);
        /* break if to_stop == 0 (we're at the logical stop)
         * AND cnt > 0 (we're not at the beginning of the
         * output block).
         */
        if (to_stop < togo) {
        if (to_stop == 0) {
            if (cnt) {
            togo = 0;
            break;
            } else /* keep togo as is: since cnt == 0, we
                    * can set the logical stop flag on this
                    * output block
                    */
            susp->logically_stopped = true;
        } else /* limit togo so we can start a new
                * block at the LST
                */
            togo = to_stop;
        }
    }

    n = togo;
    scale1_reg = susp->scale1;
    c2_reg = susp->c2;
    c1_reg = susp->c1;
    prev_reg = susp->prev;
    hz_pHaSe_ReG = susp->hz_pHaSe;
    hz_x1_sample_reg = susp->hz_x1_sample;
    s1_ptr_reg = susp->s1_ptr;
    out_ptr_reg = out_ptr;
    if (n) do { /* the inner sample computation loop */
        if (hz_pHaSe_ReG >= 1.0) {
/* fixup-depends hz */
        register double b; 
        /* pick up next sample as hz_x1_sample: */
        susp->hz_ptr++;
        susp_took(hz_cnt, 1);
        hz_pHaSe_ReG -= 1.0;
        susp_check_term_samples_break(hz, hz_ptr, hz_cnt, hz_x1_sample_reg);
        hz_x1_sample_reg = susp_current_sample(hz, hz_ptr);
        b = 2.0 - cos(hz_x1_sample_reg);
        c2_reg = susp->c2 = b - sqrt((b * b) - 1.0);
        c1_reg = susp->c1 = (1.0 - c2_reg) * scale1_reg;
        }
            *out_ptr_reg++ = (sample_type) (prev_reg = c1_reg * *s1_ptr_reg++ + c2_reg * prev_reg);
        hz_pHaSe_ReG += hz_pHaSe_iNcR_rEg;
    } while (--n); /* inner loop */

    togo -= n;
    susp->prev = prev_reg;
    susp->hz_pHaSe = hz_pHaSe_ReG;
    susp->hz_x1_sample = hz_x1_sample_reg;
    /* using s1_ptr_reg is a bad idea on RS/6000: */
    susp->s1_ptr += togo;
    out_ptr += togo;
    susp_took(s1_cnt, togo);
    cnt += togo;
    } /* outer loop */

    /* test for termination */
    if (togo == 0 && cnt == 0) {
    snd_list_terminate(snd_list);
    } else {
    snd_list->block_len = cnt;
    susp->susp.current += cnt;
    }
    /* test for logical stop */
    if (susp->logically_stopped) {
    snd_list->logically_stopped = true;
    } else if (susp->susp.log_stop_cnt == susp->susp.current) {
    susp->logically_stopped = true;
    }
} /* tonev_ni_fetch */


void tonev_nr_fetch(register tonev_susp_type susp, snd_list_type snd_list)
{
    int cnt = 0; /* how many samples computed */
    sample_type hz_val;
    int togo;
    int n;
    sample_block_type out;
    register sample_block_values_type out_ptr;

    register sample_block_values_type out_ptr_reg;

    register double scale1_reg;
    register double c2_reg;
    register double c1_reg;
    register double prev_reg;
    register sample_block_values_type s1_ptr_reg;
    falloc_sample_block(out, "tonev_nr_fetch");
    out_ptr = out->samples;
    snd_list->block = out;

    /* make sure sounds are primed with first values */
    if (!susp->started) {
    susp->started = true;
    susp->hz_pHaSe = 1.0;
    }

    susp_check_term_samples(hz, hz_ptr, hz_cnt);

    while (cnt < max_sample_block_len) { /* outer loop */
    /* first compute how many samples to generate in inner loop: */
    /* don't overflow the output sample block: */
    togo = max_sample_block_len - cnt;

    /* don't run past the s1 input sample block: */
    susp_check_term_log_samples(s1, s1_ptr, s1_cnt);
    togo = min(togo, susp->s1_cnt);

    /* grab next hz_x1_sample when phase goes past 1.0; */
    /* use hz_n (computed below) to avoid roundoff errors: */
    if (susp->hz_n <= 0) {
        register double b;
        susp_check_term_samples(hz, hz_ptr, hz_cnt);
        susp->hz_x1_sample = susp_fetch_sample(hz, hz_ptr, hz_cnt);
        susp->hz_pHaSe -= 1.0;
        /* hz_n gets number of samples before phase exceeds 1.0: */
        susp->hz_n = (long) ((1.0 - susp->hz_pHaSe) *
                    susp->output_per_hz);
        b = 2.0 - cos(susp->hz_x1_sample);
        susp->c2 = b - sqrt((b * b) - 1.0);
        susp->c1 = (1.0 - susp->c2) * susp->scale1;
    }
    togo = min(togo, susp->hz_n);
    hz_val = susp->hz_x1_sample;
    /* don't run past terminate time */
    if (susp->terminate_cnt != UNKNOWN &&
        susp->terminate_cnt <= susp->susp.current + cnt + togo) {
        togo = susp->terminate_cnt - (susp->susp.current + cnt);
        if (togo == 0) break;
    }


    /* don't run past logical stop time */
    if (!susp->logically_stopped && susp->susp.log_stop_cnt != UNKNOWN) {
        int to_stop = susp->susp.log_stop_cnt - (susp->susp.current + cnt);
        /* break if to_stop == 0 (we're at the logical stop)
         * AND cnt > 0 (we're not at the beginning of the
         * output block).
         */
        if (to_stop < togo) {
        if (to_stop == 0) {
            if (cnt) {
            togo = 0;
            break;
            } else /* keep togo as is: since cnt == 0, we
                    * can set the logical stop flag on this
                    * output block
                    */
            susp->logically_stopped = true;
        } else /* limit togo so we can start a new
                * block at the LST
                */
            togo = to_stop;
        }
    }

    n = togo;
    scale1_reg = susp->scale1;
    c2_reg = susp->c2;
    c1_reg = susp->c1;
    prev_reg = susp->prev;
    s1_ptr_reg = susp->s1_ptr;
    out_ptr_reg = out_ptr;
    if (n) do { /* the inner sample computation loop */
            *out_ptr_reg++ = (sample_type) (prev_reg = c1_reg * *s1_ptr_reg++ + c2_reg * prev_reg);
    } while (--n); /* inner loop */

    susp->prev = prev_reg;
    /* using s1_ptr_reg is a bad idea on RS/6000: */
    susp->s1_ptr += togo;
    out_ptr += togo;
    susp_took(s1_cnt, togo);
    susp->hz_pHaSe += togo * susp->hz_pHaSe_iNcR;
    susp->hz_n -= togo;
    cnt += togo;
    } /* outer loop */

    /* test for termination */
    if (togo == 0 && cnt == 0) {
    snd_list_terminate(snd_list);
    } else {
    snd_list->block_len = cnt;
    susp->susp.current += cnt;
    }
    /* test for logical stop */
    if (susp->logically_stopped) {
    snd_list->logically_stopped = true;
    } else if (susp->susp.log_stop_cnt == susp->susp.current) {
    susp->logically_stopped = true;
    }
} /* tonev_nr_fetch */


void tonev_toss_fetch(susp, snd_list)
  register tonev_susp_type susp;
  snd_list_type snd_list;
{
    long final_count = susp->susp.toss_cnt;
    time_type final_time = susp->susp.t0;
    long n;

    /* fetch samples from s1 up to final_time for this block of zeros */
    while ((round((final_time - susp->s1->t0) * susp->s1->sr)) >=
       susp->s1->current)
    susp_get_samples(s1, s1_ptr, s1_cnt);
    /* fetch samples from hz up to final_time for this block of zeros */
    while ((round((final_time - susp->hz->t0) * susp->hz->sr)) >=
       susp->hz->current)
    susp_get_samples(hz, hz_ptr, hz_cnt);
    /* convert to normal processing when we hit final_count */
    /* we want each signal positioned at final_time */
    n = round((final_time - susp->s1->t0) * susp->s1->sr -
         (susp->s1->current - susp->s1_cnt));
    susp->s1_ptr += n;
    susp_took(s1_cnt, n);
    n = round((final_time - susp->hz->t0) * susp->hz->sr -
         (susp->hz->current - susp->hz_cnt));
    susp->hz_ptr += n;
    susp_took(hz_cnt, n);
    susp->susp.fetch = susp->susp.keep_fetch;
    (*(susp->susp.fetch))(susp, snd_list);
}


void tonev_mark(tonev_susp_type susp)
{
    sound_xlmark(susp->s1);
    sound_xlmark(susp->hz);
}


void tonev_free(tonev_susp_type susp)
{
    sound_unref(susp->s1);
    sound_unref(susp->hz);
    ffree_generic(susp, sizeof(tonev_susp_node), "tonev_free");
}


void tonev_print_tree(tonev_susp_type susp, int n)
{
    indent(n);
    stdputstr("s1:");
    sound_print_tree_1(susp->s1, n);

    indent(n);
    stdputstr("hz:");
    sound_print_tree_1(susp->hz, n);
}


sound_type snd_make_tonev(sound_type s1, sound_type hz)
{
    register tonev_susp_type susp;
    rate_type sr = s1->sr;
    time_type t0 = max(s1->t0, hz->t0);
    int interp_desc = 0;
    sample_type scale_factor = 1.0F;
    time_type t0_min = t0;
    falloc_generic(susp, tonev_susp_node, "snd_make_tonev");
    susp->scale1 = s1->scale;
    susp->c2 = 0.0;
    susp->c1 = 0.0;
    susp->prev = 0.0;
    hz->scale = (sample_type) (hz->scale * (PI2 / s1->sr));

    /* select a susp fn based on sample rates */
    interp_desc = (interp_desc << 2) + interp_style(s1, sr);
    interp_desc = (interp_desc << 2) + interp_style(hz, sr);
    switch (interp_desc) {
      case INTERP_sn: /* handled below */
      case INTERP_ss: /* handled below */
      case INTERP_nn: /* handled below */
      case INTERP_ns: susp->susp.fetch = tonev_ns_fetch; break;
      case INTERP_si: /* handled below */
      case INTERP_ni: susp->susp.fetch = tonev_ni_fetch; break;
      case INTERP_sr: /* handled below */
      case INTERP_nr: susp->susp.fetch = tonev_nr_fetch; break;
    }

    susp->terminate_cnt = UNKNOWN;
    /* handle unequal start times, if any */
    if (t0 < s1->t0) sound_prepend_zeros(s1, t0);
    if (t0 < hz->t0) sound_prepend_zeros(hz, t0);
    /* minimum start time over all inputs: */
    t0_min = min(s1->t0, min(hz->t0, t0));
    /* how many samples to toss before t0: */
    susp->susp.toss_cnt = (long) ((t0 - t0_min) * sr + 0.5);
    if (susp->susp.toss_cnt > 0) {
    susp->susp.keep_fetch = susp->susp.fetch;
    susp->susp.fetch = tonev_toss_fetch;
    }

    /* initialize susp state */
    susp->susp.free = tonev_free;
    susp->susp.sr = sr;
    susp->susp.t0 = t0;
    susp->susp.mark = tonev_mark;
    susp->susp.print_tree = tonev_print_tree;
    susp->susp.name = "tonev";
    susp->logically_stopped = false;
    susp->susp.log_stop_cnt = logical_stop_cnt_cvt(s1);
    susp->started = false;
    susp->susp.current = 0;
    susp->s1 = s1;
    susp->s1_cnt = 0;
    susp->hz = hz;
    susp->hz_cnt = 0;
    susp->hz_pHaSe = 0.0;
    susp->hz_pHaSe_iNcR = hz->sr / sr;
    susp->hz_n = 0;
    susp->output_per_hz = sr / hz->sr;
    return sound_create((snd_susp_type)susp, t0, sr, scale_factor);
}


sound_type snd_tonev(sound_type s1, sound_type hz)
{
    sound_type s1_copy = sound_copy(s1);
    sound_type hz_copy = sound_copy(hz);
    return snd_make_tonev(s1_copy, hz_copy);
}
