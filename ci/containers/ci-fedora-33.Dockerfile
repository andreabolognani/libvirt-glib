# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool dockerfile fedora-33 libvirt+dist,libvirt-glib
#
# https://gitlab.com/libvirt/libvirt-ci/-/commit/b098ec6631a85880f818f2dd25c437d509e53680
FROM registry.fedoraproject.org/fedora:33

RUN dnf install -y nosync && \
    echo -e '#!/bin/sh\n\
if test -d /usr/lib64\n\
then\n\
    export LD_PRELOAD=/usr/lib64/nosync/nosync.so\n\
else\n\
    export LD_PRELOAD=/usr/lib/nosync/nosync.so\n\
fi\n\
exec "$@"' > /usr/bin/nosync && \
    chmod +x /usr/bin/nosync && \
    nosync dnf update -y && \
    nosync dnf install -y \
        autoconf \
        automake \
        ca-certificates \
        ccache \
        gcc \
        gettext \
        gettext-devel \
        git \
        glib2-devel \
        glibc-langpack-en \
        gobject-introspection-devel \
        gtk-doc \
        libtool \
        libvirt-devel \
        libxml2-devel \
        make \
        pkgconfig \
        rpm-build \
        vala && \
    nosync dnf autoremove -y && \
    nosync dnf clean all -y && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"