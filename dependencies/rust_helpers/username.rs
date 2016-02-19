use std::ffi::CString;
use std::env;


extern {
	fn memcpy(dest: *mut i8, src: *const i8, len: u64) -> *const i8;
}


#[no_mangle]
pub extern "C" fn rs_username(buf: *mut i8, len: u64) -> i32 {
	match env::home_dir().and_then(|pbuf| pbuf.as_path().file_name().and_then(|fname| fname.to_str().map(|string| string.to_string()))) {
		Some(string) => {
			let to_copy = [len, string.len() as u64].iter().cloned().min().unwrap();

			unsafe {
				memcpy(buf, CString::new(string.as_bytes()).unwrap().into_raw(), to_copy);
			}

			to_copy as i32
		}
		None => -1,
	}
}
