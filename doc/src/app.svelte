<script>
  import { Route } from 'tinro';

  import HomePage     from './page/home.svelte';
  import NotFoundPage from './page/not-found.svelte';

  // Emulate optional prefix
  let prefix = '/*';
  if (document.location.host == 'finwo.github.io') {
    prefix = '/pmlag/*';
  }

</script>

<Route path={prefix} >
  <Route path="/"><HomePage/></Route>
  <Route fallback><NotFoundPage/></Route>
</Route>
