<script>
  import { Route } from 'svelte-router-spa';
  export let currentRoute;
  const params = {};

  console.log({ currentRoute });
</script>

<main>
  <Route {currentRoute}  {params} />
</main>
