# Vendored Phlex header (stopgap)

`phlex/driver.hpp` here is a **verbatim copy** of the header from the Phlex
v0.3.0 release (`reference/phlex/phlex/driver.hpp`).

Phlex v0.3.0 does not *install* `phlex/driver.hpp` (its `phlex/CMakeLists.txt`
installs only `concurrency.hpp module.hpp source.hpp`), even though
`PHLEX_REGISTER_DRIVER` is a release-notes-public API and all of `driver.hpp`'s
dependencies are installed. This copy lets `DriverModule.cpp` (the file-driven
read driver, ddm-c3s.16) compile against the installed Phlex.

It is used **only when the installed `phlex/driver.hpp` is absent**: the
CMakeLists adds this `vendor/` directory to the driver plugin's include path
only if `find_file(phlex/driver.hpp)` fails. Once Phlex ships the header
(ddm-c3s.19), the installed one is used automatically and this copy should be
deleted. Keep it byte-identical to upstream so the diff stays trivial.
