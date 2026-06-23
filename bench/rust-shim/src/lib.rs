use minibwa::{Aligner, Index, Meth, Opts, ThreadBuf};
use std::ffi::CStr;
use std::os::raw::c_char;
use std::sync::{Mutex, OnceLock};

pub struct BenchAligner {
    idx: Index,
    opts: Opts,
}

static BENCH: OnceLock<Mutex<Option<BenchAligner>>> = OnceLock::new();

fn bench_slot() -> &'static Mutex<Option<BenchAligner>> {
    BENCH.get_or_init(|| Mutex::new(None))
}

unsafe fn cstr<'a>(p: *const c_char) -> &'a str {
    assert!(!p.is_null(), "null C string");
    CStr::from_ptr(p).to_str().expect("C string is not UTF-8")
}

unsafe fn first_cstr<'a>(p: *mut *const c_char) -> &'a str {
    assert!(!p.is_null(), "null C string vector");
    cstr(*p)
}

#[no_mangle]
pub unsafe extern "C" fn rminibwa_bench_rust_init_c(prefix: *mut *const c_char) {
    let prefix = first_cstr(prefix);
    let idx = Index::load(prefix, false).expect("failed to load minibwa index");
    let opts = Opts::with_preset("sr").expect("failed to create sr opts").set_out_n(0);
    *bench_slot().lock().expect("rust benchmark mutex poisoned") = Some(BenchAligner { idx, opts });
}

#[no_mangle]
pub unsafe extern "C" fn rminibwa_bench_rust_map_count_c(query: *mut *const c_char, out: *mut i32) {
    assert!(!out.is_null(), "null output pointer");
    let query = first_cstr(query);
    let mut guard = bench_slot().lock().expect("rust benchmark mutex poisoned");
    let h = guard.as_mut().expect("rust benchmark handle is not initialized");
    let aligner = Aligner::new(&h.idx, &h.opts);
    let mut buf = ThreadBuf::new();
    *out = aligner
        .map(&mut buf, b"read1", query.as_bytes(), Meth::None)
        .expect("rust minibwa map failed")
        .len() as i32;
}

#[no_mangle]
pub unsafe extern "C" fn rminibwa_bench_rust_clear_c() {
    *bench_slot().lock().expect("rust benchmark mutex poisoned") = None;
}

#[no_mangle]
pub unsafe extern "C" fn rminibwa_bench_rust_new(prefix: *const c_char) -> *mut BenchAligner {
    let prefix = cstr(prefix);
    let idx = Index::load(prefix, false).expect("failed to load minibwa index");
    let opts = Opts::with_preset("sr").expect("failed to create sr opts").set_out_n(0);
    Box::into_raw(Box::new(BenchAligner { idx, opts }))
}

#[no_mangle]
pub unsafe extern "C" fn rminibwa_bench_rust_map_count(handle: *mut BenchAligner, query: *const c_char) -> i32 {
    assert!(!handle.is_null(), "null BenchAligner handle");
    let query = cstr(query);
    let h = &mut *handle;
    let aligner = Aligner::new(&h.idx, &h.opts);
    let mut buf = ThreadBuf::new();
    aligner
        .map(&mut buf, b"read1", query.as_bytes(), Meth::None)
        .expect("rust minibwa map failed")
        .len() as i32
}

#[no_mangle]
pub unsafe extern "C" fn rminibwa_bench_rust_free(handle: *mut BenchAligner) {
    if !handle.is_null() {
        drop(Box::from_raw(handle));
    }
}
