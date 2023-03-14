<script>
  import { Route } from 'tinro';

  import HomePage     from './page/home.svelte';
  import NotFoundPage from './page/not-found.svelte';
</script>

<Route path="/"><HomePage/></Route>
<Route fallback><NotFoundPage/></Route>
