typedef u_int ips<>;

program CFGSRVPROG {
  version CFGSRVVERS {
    ips GET_HOSTS_BY_KEY(string) = 1;
  } = 1;
} = 1;
