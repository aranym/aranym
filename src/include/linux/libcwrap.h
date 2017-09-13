/* glibc bindings for target ABI version glibc 2.11 */
#if defined(__linux__) && !defined (__LIBC_CUSTOM_BINDINGS_H__) && !defined(__ANDROID__)

#if defined (__cplusplus)
extern "C" {
#endif

#undef SYMVER
#undef SYMVER1
#ifdef __ASSEMBLER__
#define SYMVER1(name, ver) .symver name, name##@##ver
#else
#define SYMVER1(name, ver) __asm__(".symver " #name ", " #name "@" #ver );
#endif
#define SYMVER(name, ver) SYMVER1(name, ver)

#if defined(__i386__) || defined(__x86_64__)
 
/* Symbols redirected to earlier glibc versions */
SYMVER(__longjmp_chk, GLIBC_2.11)
SYMVER(_sys_errlist, GLIBC_2.4)
SYMVER(_sys_nerr, GLIBC_2.4)
SYMVER(clock_getcpuclockid, GLIBC_2.2.5)
SYMVER(clock_getres, GLIBC_2.2.5)
SYMVER(clock_gettime, GLIBC_2.2.5)
SYMVER(clock_nanosleep, GLIBC_2.2.5)
SYMVER(clock_settime, GLIBC_2.2.5)
SYMVER(execvpe, GLIBC_2.11)
SYMVER(fmemopen, GLIBC_2.2.5)
SYMVER(lgamma, GLIBC_2.2.5)
SYMVER(lgammaf, GLIBC_2.2.5)
SYMVER(lgammal, GLIBC_2.2.5)
SYMVER(memcpy, GLIBC_2.2.5)
SYMVER(mkostemps, GLIBC_2.11)
SYMVER(mkostemps64, GLIBC_2.11)
SYMVER(mkstemps, GLIBC_2.11)
SYMVER(mkstemps64, GLIBC_2.11)
SYMVER(posix_spawn, GLIBC_2.2.5)
SYMVER(posix_spawnp, GLIBC_2.2.5)
SYMVER(pthread_sigqueue, GLIBC_2.11)
SYMVER(quick_exit, GLIBC_2.10)
SYMVER(sys_errlist, GLIBC_2.4)
SYMVER(sys_nerr, GLIBC_2.4)

/* Symbols introduced in newer glibc versions, which must not be used */
SYMVER(_ZGVbN2v_cos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN2v_exp, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN2v_log, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN2v_sin, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN2vv_pow, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN2vvv_sincos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN4v_cosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN4v_expf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN4v_logf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN4v_sinf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN4vv_powf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVbN4vvv_sincosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN4v_cos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN4v_exp, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN4v_log, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN4v_sin, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN4vv_pow, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN4vvv_sincos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN8v_cosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN8v_expf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN8v_logf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN8v_sinf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN8vv_powf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVcN8vvv_sincosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN4v_cos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN4v_exp, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN4v_log, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN4v_sin, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN4vv_pow, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN4vvv_sincos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN8v_cosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN8v_expf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN8v_logf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN8v_sinf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN8vv_powf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVdN8vvv_sincosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN16v_cosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN16v_expf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN16v_logf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN16v_sinf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN16vv_powf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN16vvv_sincosf, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN8v_cos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN8v_exp, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN8v_log, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN8v_sin, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN8vv_pow, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(_ZGVeN8vvv_sincos, GLIBC_DONT_USE_THIS_VERSION_2.22)
SYMVER(__acos_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__acosf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__acosf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__acosh_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__acoshf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__acoshf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__acoshl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__acosl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__asin_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__asinf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__asinf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__asinl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__atan2_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__atan2f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__atan2f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__atan2l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__atanh_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__atanhf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__atanhf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__atanhl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__cosh_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__coshf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__coshf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__coshl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__cxa_thread_atexit_impl, GLIBC_DONT_USE_THIS_VERSION_2.18)
SYMVER(__exp10_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__exp10f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__exp10f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__exp10l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__exp2_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__exp2f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__exp2f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__exp2l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__exp_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__expf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__expf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__expl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__explicit_bzero_chk, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(__fdelt_chk, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__fdelt_warn, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__fentry__, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(__finitef128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__fmod_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__fmodf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__fmodf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__fmodl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__fpclassifyf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__gamma_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__gammaf128_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__gammaf_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__gammal_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__getauxval, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(__hypot_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__hypotf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__hypotf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__hypotl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__iscanonicall, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(__iseqsig, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(__iseqsigf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(__iseqsigf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__iseqsigl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(__isinff128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__isnanf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__issignaling, GLIBC_DONT_USE_THIS_VERSION_2.18)
SYMVER(__issignalingf, GLIBC_DONT_USE_THIS_VERSION_2.18)
SYMVER(__issignalingf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__issignalingl, GLIBC_DONT_USE_THIS_VERSION_2.18)
SYMVER(__j0_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__j0f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__j0f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__j0l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__j1_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__j1f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__j1f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__j1l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__jn_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__jnf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__jnf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__jnl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__lgamma_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__lgammaf128_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__lgammaf_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__lgammal_r_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__log10_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__log10f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__log10f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__log10l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__log2_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__log2f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__log2f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__log2l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__log_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__logf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__logf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__logl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__poll_chk, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(__pow_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__powf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__powf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__powl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__ppoll_chk, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(__remainder_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__remainderf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__remainderf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__remainderl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__scalb_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__scalbf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__scalbl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__signbitf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__signgam, GLIBC_DONT_USE_THIS_VERSION_2.23)
SYMVER(__sinh_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__sinhf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__sinhf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__sinhl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__sqrt_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__sqrtf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__sqrtf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__sqrtl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__strtof128_internal, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__wcstof128_internal, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__y0_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__y0f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__y0f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__y0l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__y1_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__y1f128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__y1f_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__y1l_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__yn_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__ynf128_finite, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(__ynf_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(__ynl_finite, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(acosf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(acoshf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(aligned_alloc, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(asinf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(asinhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(atan2f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(atanf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(atanhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(c16rtomb, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(c32rtomb, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(cabsf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cacosf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cacoshf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(canonicalize, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(canonicalizef, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(canonicalizef128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(canonicalizel, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(cargf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(casinf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(casinhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(catanf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(catanhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cbrtf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ccosf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ccoshf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ceilf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cexpf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cimagf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(clock_adjtime, GLIBC_DONT_USE_THIS_VERSION_2.14)
SYMVER(clog10f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(clogf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(conjf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(copysignf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cosf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(coshf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cpowf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(cprojf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(crealf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(csinf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(csinhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(csqrtf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ctanf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ctanhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(erfcf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(erff128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(exp10f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(exp2f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(expf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(explicit_bzero, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(expm1f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fabsf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fanotify_init, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(fanotify_mark, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(fdimf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fegetmode, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fesetexcept, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fesetmode, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fetestexceptflag, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(floorf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fmaf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fmaxf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fmaxmag, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fmaxmagf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fmaxmagf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fmaxmagl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fminf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fminmag, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fminmagf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fminmagf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fminmagl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fmodf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(frexpf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fromfp, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fromfpf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fromfpf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fromfpl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fromfpx, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fromfpxf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fromfpxf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(fromfpxl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(fts64_children, GLIBC_DONT_USE_THIS_VERSION_2.23)
SYMVER(fts64_close, GLIBC_DONT_USE_THIS_VERSION_2.23)
SYMVER(fts64_open, GLIBC_DONT_USE_THIS_VERSION_2.23)
SYMVER(fts64_read, GLIBC_DONT_USE_THIS_VERSION_2.23)
SYMVER(fts64_set, GLIBC_DONT_USE_THIS_VERSION_2.23)
SYMVER(getauxval, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(getentropy, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(getpayload, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(getpayloadf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(getpayloadf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(getpayloadl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(getrandom, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(hypotf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ilogbf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(j0f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(j1f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(jnf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ldexpf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(lgammaf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(lgammaf128_r, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(llogb, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(llogbf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(llogbf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(llogbl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(llrintf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(llroundf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(log10f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(log1pf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(log2f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(logbf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(logf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(lrintf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(lroundf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(mbrtoc16, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(mbrtoc32, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(modff128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(name_to_handle_at, GLIBC_DONT_USE_THIS_VERSION_2.14)
SYMVER(nanf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(nearbyintf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(nextafterf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(nextdown, GLIBC_DONT_USE_THIS_VERSION_2.24)
SYMVER(nextdownf, GLIBC_DONT_USE_THIS_VERSION_2.24)
SYMVER(nextdownf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(nextdownl, GLIBC_DONT_USE_THIS_VERSION_2.24)
SYMVER(nextup, GLIBC_DONT_USE_THIS_VERSION_2.24)
SYMVER(nextupf, GLIBC_DONT_USE_THIS_VERSION_2.24)
SYMVER(nextupf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(nextupl, GLIBC_DONT_USE_THIS_VERSION_2.24)
SYMVER(ntp_gettimex, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(open_by_handle_at, GLIBC_DONT_USE_THIS_VERSION_2.14)
SYMVER(powf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(preadv2, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(preadv64v2, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(prlimit, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(prlimit64, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(process_vm_readv, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(process_vm_writev, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(pthread_getattr_default_np, GLIBC_DONT_USE_THIS_VERSION_2.18)
SYMVER(pthread_getname_np, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_mutex_consistent, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_mutexattr_getrobust, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_mutexattr_setrobust, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_setattr_default_np, GLIBC_DONT_USE_THIS_VERSION_2.18)
SYMVER(pthread_setname_np, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pwritev2, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(pwritev64v2, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(reallocarray, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(recvmmsg, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(remainderf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(remquof128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(rintf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(roundeven, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(roundevenf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(roundevenf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(roundevenl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(roundf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(scalblnf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(scalbnf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(scandirat, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(scandirat64, GLIBC_DONT_USE_THIS_VERSION_2.15)
SYMVER(secure_getenv, GLIBC_DONT_USE_THIS_VERSION_2.17)
SYMVER(sendmmsg, GLIBC_DONT_USE_THIS_VERSION_2.14)
SYMVER(setns, GLIBC_DONT_USE_THIS_VERSION_2.14)
SYMVER(setpayload, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(setpayloadf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(setpayloadf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(setpayloadl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(setpayloadsig, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(setpayloadsigf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(setpayloadsigf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(setpayloadsigl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(sincosf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(sinf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(sinhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(sqrtf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(strfromd, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(strfromf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(strfromf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(strfroml, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(strtof128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(strtof128_l, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(syncfs, GLIBC_DONT_USE_THIS_VERSION_2.14)
SYMVER(tanf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(tanhf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(tgammaf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(timespec_get, GLIBC_DONT_USE_THIS_VERSION_2.16)
SYMVER(totalorder, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(totalorderf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(totalorderf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(totalorderl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(totalordermag, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(totalordermagf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(totalordermagf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(totalordermagl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(truncf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ufromfp, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(ufromfpf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(ufromfpf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ufromfpl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(ufromfpx, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(ufromfpxf, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(ufromfpxf128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ufromfpxl, GLIBC_DONT_USE_THIS_VERSION_2.25)
SYMVER(wcstof128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(wcstof128_l, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(y0f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(y1f128, GLIBC_DONT_USE_THIS_VERSION_2.26)
SYMVER(ynf128, GLIBC_DONT_USE_THIS_VERSION_2.26)

#endif /* x86 */


#if defined(__arm__) || defined(__aarch64__)

/* Symbols redirected to earlier glibc versions */
SYMVER(__longjmp_chk, GLIBC_2.11)
SYMVER(_sys_errlist, GLIBC_2.4)
SYMVER(_sys_nerr, GLIBC_2.4)
SYMVER(execvpe, GLIBC_2.11)
SYMVER(fallocate64, GLIBC_2.11)
SYMVER(mkostemps, GLIBC_2.11)
SYMVER(mkostemps64, GLIBC_2.11)
SYMVER(mkstemps, GLIBC_2.11)
SYMVER(mkstemps64, GLIBC_2.11)
SYMVER(pthread_sigqueue, GLIBC_2.11)
SYMVER(sys_errlist, GLIBC_2.4)
SYMVER(sys_nerr, GLIBC_2.4)

/* Symbols introduced in newer glibc versions, which must not be used */
SYMVER(fanotify_init, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(fanotify_mark, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(ntp_gettimex, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(prlimit, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(prlimit64, GLIBC_DONT_USE_THIS_VERSION_2.13)
SYMVER(pthread_getname_np, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_mutex_consistent, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_mutexattr_getrobust, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_mutexattr_setrobust, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(pthread_setname_np, GLIBC_DONT_USE_THIS_VERSION_2.12)
SYMVER(recvmmsg, GLIBC_DONT_USE_THIS_VERSION_2.12)

#endif /* __arm__ */

#undef SYMVER
#undef SYMVER1

#if defined (__cplusplus)
}
#endif
#endif
