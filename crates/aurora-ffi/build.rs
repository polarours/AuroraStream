use std::env;
use std::path::PathBuf;

fn main() {
    let workspace_root = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .parent()
        .unwrap()
        .parent()
        .unwrap()
        .to_path_buf();

    let cpp_dir = workspace_root.join("cpp");

    let dst = cmake::build(&cpp_dir);
    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:rustc-link-lib=dylib=aurora_media");
    println!("cargo:rerun-if-changed=cpp/src/");
    println!("cargo:rerun-if-changed=cpp/include/");
    println!("cargo:rerun-if-changed=cpp/CMakeLists.txt");

    let header = cpp_dir.join("include").join("aurora_media.h");
    let bindings_out = PathBuf::from(env::var("OUT_DIR").unwrap()).join("bindings.rs");

    let bindings = bindgen::Builder::default()
        .header(header.to_str().unwrap())
        .clang_arg("-I".to_string() + cpp_dir.join("include").to_str().unwrap())
        .derive_debug(true)
        .derive_default(true)
        .derive_partialeq(true)
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file(&bindings_out)
        .expect("Couldn't write bindings!");
    println!("cargo:rerun-if-changed={}", header.display());
}
