add_rules("mode.debug", "mode.release")

add_repositories("zero https://github.com/zeromake/xrepo.git")
add_requires("zeromake.rules")
add_requires("fwlib_stc8", {configs={model="STC8G1K08"}})

target("boot")
    add_files("main.c")
    add_packages("zeromake.rules", "fwlib_stc8")
    add_rules("@zeromake.rules/mcs51", {model = "STC8G1K08"})
